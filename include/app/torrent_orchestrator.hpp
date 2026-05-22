#pragma once

#include "app/peer_manager.hpp"
#include "app/piece_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <string>

class TorrentOrchestrator {
public:
    explicit TorrentOrchestrator(std::string path, std::filesystem::path outputPath, bool logging,
                                 std::string expectedSha256 = std::string());
    void download();

private:
    bt::core::TorrentMetadata _metadata;
    std::filesystem::path _outputPath;
    bool _logging = false;
    std::string _expectedSha256;

    std::unique_ptr<bt::PeerManager> _peerManager;
    std::shared_ptr<bt::PieceManager> _pieceManager;

    std::mutex _completionMutex;
    std::condition_variable cv;

    // PiecesManager
    //...
};