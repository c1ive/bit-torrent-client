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
