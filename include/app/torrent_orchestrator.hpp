#include "core/torrent_metadata_loader.hpp"
#include <string>

class TorrentOrchestrator {
private:
    bt::core::TorrentMetadata _metadata;
    bt::core::TorrentMetadata _loadMetadata(const std::string_view path) const;

    // PeerManager
    // PiecesManager
    //...

public:
    TorrentOrchestrator(std::string path);
    void start();
};