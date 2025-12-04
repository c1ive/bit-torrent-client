#pragma once

#include "app/peer_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <optional>
#include <string>

class TorrentOrchestrator {
private:
    bt::core::TorrentMetadata _metadata;

    std::optional<bt::PeerManager> _peerManager;
    // PiecesManager
    //...

public:
    explicit TorrentOrchestrator(std::string path);
    void start();
};