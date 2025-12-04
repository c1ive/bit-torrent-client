#include "app/peer_manager.hpp"
#include <algorithm>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <vector>

namespace bt {
PeerManager::PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer)
    : _peers{_deserializePeerBuffer(peerBuffer)} {}

void PeerManager::start() const {
    spdlog::debug("Starting the peer manager...");
    for (const auto& peer : _peers) {
        spdlog::debug("Found Peer: {}.{}.{}.{}:{}", static_cast<unsigned>(peer.ip[0]),
                      static_cast<unsigned>(peer.ip[1]), static_cast<unsigned>(peer.ip[2]),
                      static_cast<unsigned>(peer.ip[3]), peer.port);
    }
};

std::vector<PeerManager::Peer>
PeerManager::_deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer) {
    std::vector<Peer> peers;
    spdlog::debug("Deserializing peer buffer...");

    for (const auto& peer : peerBuffer) {
        const auto iter = peer.begin();
        IpAddr ip;
        std::copy_n(iter, 4, ip.data());

        uint16_t port = static_cast<uint16_t>((static_cast<uint16_t>(peer[4]) << 8) |
                                              static_cast<uint16_t>(peer[5]));

        peers.push_back({.port = port, .ip = ip});
    }

    return peers;
}
} // namespace bt
