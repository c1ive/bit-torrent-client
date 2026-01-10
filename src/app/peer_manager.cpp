#include "app/peer_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <algorithm>
#include <asio/detail/handler_work.hpp>
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

    // Create a new PeerSession
    core::PeerSession session(_ctx);

    if (session.connect(_peers[0])) {
        spdlog::info("Connected to peer! Sending handshake...");

        // 2. Run the test (Send data + Block until response received)
        try {
            session.doHandshake(_infoHash, _peerId);
            spdlog::info("Handshake executed successfully.");
        } catch (const std::exception& e) {
            spdlog::error("Error during handshake test: {}", e.what());
        }
    } else {
        spdlog::error("Failed to connect to peer.");
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
