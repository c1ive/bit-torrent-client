#include "app/peer_manager.hpp"
#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <algorithm>
#include <asio/detail/handler_work.hpp>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <vector>

namespace bt {
PeerSession::PeerSession(asio::io_context& io_context) : _socket(io_context) {}

bool PeerSession::connect(const core::Peer& peer) {
    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(peer.getIpStr()), peer.port);
    spdlog::debug("Connecting to peer at {}:{}", peer.getIpStr(), peer.port);

    try {
        _socket.connect(endpoint);
        spdlog::debug("Successfully connected to peer at {}:{}", peer.getIpStr(), peer.port);
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to peer at {}:{} - {}", peer.getIpStr(), peer.port,
                      e.what());
        return false;
    }

    return true;
}

void PeerSession::doHandshake(const core::Sha1Hash& infoHash, std::string_view peerId) {
    spdlog::debug("Performing handshake...");
    core::HandshakeMsg handshake = core::serializeHandshake(infoHash, peerId);

    spdlog::debug("Sending buffer to peer.");
    _socket.send(asio::buffer(handshake));

    std::array<uint8_t, core::msg::HANDSHAKE_LEN> handshakeResponse{};
    asio::read(_socket, asio::buffer(handshakeResponse));
    spdlog::debug("Read {} bytes from peer", handshakeResponse.size());

    if (!core::verifyHandshake(handshakeResponse, infoHash)) {
        throw std::runtime_error{"Handshake verification failed."};
    }

    spdlog::debug("Handshake completed with {}:{}.",
                  _socket.remote_endpoint().address().to_string(),
                  _socket.remote_endpoint().port());
}

PeerManager::PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer, core::Sha1Hash& infoHash,
                         std::string_view peerId)
    : _ctx(), _peers{_deserializePeerBuffer(peerBuffer)}, _infoHash(infoHash), _peerId(peerId) {}

void PeerManager::start() {
    spdlog::debug("Starting the peer manager...");
    for (const auto& peer : _peers) {
        spdlog::debug("Found Peer: {}:{}", peer.getIpStr(), peer.port);
    }

    std::vector<PeerSession> sessions{};

    for (const auto& peer : _peers) {
        PeerSession session{_ctx};
        try {
            session.connect(peer);
            sessions.emplace_back(std::move(session));
        } catch (std::runtime_error e) {
            spdlog::warn("%s", e.what());
        }
    }

    for (auto& session : sessions) {
        try {
            session.doHandshake(_infoHash, _peerId);
            spdlog::info("Successfully completed handshake with peer.");
        } catch (std::runtime_error e) {
            spdlog::warn("{}", e.what());
        }
    }
};

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
