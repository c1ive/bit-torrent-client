#pragma once

#include "app/peer_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <string>

class TorrentOrchestrator {
private:
    bt::core::TorrentMetadata _metadata;

    std::unique_ptr<bt::PeerManager> _peerManager;
    // PiecesManager
    //...

public:
    explicit TorrentOrchestrator(std::string path);
    void start();
};