#pragma once

#include "app/peer_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <string>

class TorrentOrchestrator {
private:
    bt::core::TorrentMetadata _metadata;
    bt::core::TorrentMetadata _loadMetadata(const std::string_view path) const;

    std::optional<bt::PeerManager> _peerManager;
    // PiecesManager
    //...

public:
    TorrentOrchestrator(std::string path);
    void start();
};