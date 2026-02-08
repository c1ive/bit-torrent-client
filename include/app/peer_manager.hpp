#pragma once
#include "app/piece_manager.hpp"
#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <array>
#include <asio/detail/handler_work.hpp>
#include <asio/io_context.hpp>
#include <cstdint>
#include <vector>

namespace bt {
class PeerManager {
public:
    PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer, core::Sha1Hash& infoHash,
                std::string_view peerId);
    ~PeerManager() = default;

    void start(std::shared_ptr<PieceManager> pieceManager);
    void stop();

private:
    asio::io_context _ctx;
    std::vector<core::Peer> _peers;
    core::Sha1Hash& _infoHash;
    std::string_view _peerId;
    std::vector<std::thread> _threadPool;

    static std::vector<core::Peer>
    _deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer);
};

} // namespace bt