#pragma once

#include "app/peer_manager.hpp"
#include "app/piece_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <condition_variable>
#include <memory>
#include <string>

class TorrentOrchestrator {
public:
    explicit TorrentOrchestrator(std::string path);
    void download();

private:
    bt::core::TorrentMetadata _metadata;

    std::unique_ptr<bt::PeerManager> _peerManager;
    std::shared_ptr<bt::PieceManager> _pieceManager;

    std::mutex _completionMutex;
    std::condition_variable cv;

    // PiecesManager
    //...
};