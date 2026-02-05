#include "app/peer_manager.hpp"
#include "app/peer_session.hpp"
#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <algorithm>
#include <asio/detail/handler_work.hpp>
#include <asio/error_code.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <vector>

namespace bt {
PeerSession::PeerSession(asio::io_context& io_context) : _socket(io_context) {}

asio::awaitable<void> PeerSession::connect(const core::Peer& peer) {
    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(peer.getIpStr()), peer.port);
    spdlog::debug("Connecting to peer at {}:{}", peer.getIpStr(), peer.port);

    auto [ec] = co_await _socket.async_connect(endpoint, asio::as_tuple(asio::use_awaitable));

    if (ec) {
        spdlog::debug("Failed to connect to peer at {}:{} - {}", peer.getIpStr(), peer.port,
                      ec.message());
        _state = PeerState::ERROR;
        co_return;
    }
    spdlog::debug("Successfully connected to peer at {}:{}", peer.getIpStr(), peer.port);
}

asio::awaitable<void> PeerSession::doHandshake(const core::Sha1Hash& infoHash,
                                               std::string_view peerId) {
    spdlog::debug("Performing handshake...");
    core::HandshakeMsg handshake = core::serializeHandshake(infoHash, peerId);

    spdlog::debug("Sending buffer to peer.");
    auto [ec1, len] = co_await asio::async_write(_socket, asio::buffer(handshake),
                                                 asio::as_tuple(asio::use_awaitable));
    if (ec1) {
        spdlog::error("Sending handshake failed: {}", ec1.message());
        _state = PeerState::ERROR;
        co_return;
    }
    if (len != handshake.size()) {
        spdlog::error("Failed to send whole handshake buffer to peer");
        _state = PeerState::ERROR;
        co_return;
    }

    spdlog::debug("Handshake sent, waiting for response...");
    std::array<uint8_t, core::msg::HANDSHAKE_LEN> handshakeResponse{};

    auto [ec2, bytes_transferred] = co_await asio::async_read(
        _socket, asio::buffer(handshakeResponse), asio::as_tuple(asio::use_awaitable));

    spdlog::debug("Read {} bytes from peer", bytes_transferred);
    if (ec2) {
        spdlog::debug("Failed to read handshake: {}", ec2.message());
        _state = PeerState::ERROR;
        co_return;
    }

    if (!core::verifyHandshake(handshakeResponse, infoHash)) {
        // Its verry common that a handshake with a peer fails, so its only logged in debug mode
        spdlog::debug("Handshake verification failed.");
        _state = PeerState::ERROR;
        co_return;
    }

    spdlog::info("Handshake successfully completed with {}:{}.",
                 _socket.remote_endpoint().address().to_string(), _socket.remote_endpoint().port());
    _state = PeerState::HANDSHAKE_COMPLETE;
}

PeerManager::PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer, core::Sha1Hash& infoHash,
                         std::string_view peerId)
    : _ctx(), _peers{_deserializePeerBuffer(peerBuffer)}, _infoHash(infoHash), _peerId(peerId) {}

void PeerManager::start() {
    spdlog::debug("Starting the peer manager...");
    for (const auto& peer : _peers) {
        spdlog::debug("Found Peer: {}:{}", peer.getIpStr(), peer.port);
    }

    for (const auto& peer : _peers) {
        auto session = std::make_shared<PeerSession>(_ctx);

        asio::co_spawn(
            _ctx,
            [session, peer, this]() -> asio::awaitable<void> {
                try {
                    co_await session->connect(peer);
                    co_await session->doHandshake(_infoHash, _peerId);
                    co_await session->run();
                } catch (const std::exception& e) {
                    spdlog::warn("Peer session error: {}", e.what());
                }
            },
            asio::detached);
    }

    _thread = std::thread{[this] { _ctx.run(); }};
}

void PeerManager::stop() {
    spdlog::info("Stopping peermanager...");
    _ctx.stop();
    _thread.join();
}

std::vector<core::Peer>
PeerManager::_deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer) {
    std::vector<core::Peer> peers;
    spdlog::debug("Deserializing peer buffer...");

    for (const auto& peer : peerBuffer) {
        const auto iter = peer.begin();
        core::IpAddr ip;
        std::copy_n(iter, 4, ip.data());

        uint16_t port = static_cast<uint16_t>((static_cast<uint8_t>(peer[4]) << 8) |
                                              static_cast<uint8_t>(peer[5]));

        peers.push_back({.port = port, .ip = ip});
    }

    return peers;
}
} // namespace bt
