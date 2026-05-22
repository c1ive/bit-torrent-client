#include "app/torrent_orchestrator.hpp"
#include "app/progress_tracker.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "core/tracker_communicator.hpp"
#include "core/utils.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

using namespace bt;

TorrentOrchestrator::TorrentOrchestrator(std::string path, std::filesystem::path outputPath,
                                         bool logging, std::string expectedSha256)
    : _logging(logging), _metadata(core::parseTorrentData(path)),
      _outputPath(std::move(outputPath)), _expectedSha256(std::move(expectedSha256)) {};

void TorrentOrchestrator::download() {
    // TODO: Move to peer manager
    const auto peerId = core::generateId(20);
    spdlog::debug("Generated peer id: %s", peerId);
    const auto trackerResponse = core::announceAndGetPeers(_metadata, peerId);
    auto peers = trackerResponse.peersBlob;

    std::unique_ptr<bt::ProgressTracker> p = nullptr;

    if (!_logging) {
        spdlog::set_level(spdlog::level::off);
        p = std::make_unique<bt::ProgressTracker>(_metadata.info.pieceHashes.size(), 100);
    } else {
        spdlog::set_level(spdlog::level::debug);
    }

    std::filesystem::path outputPath = _outputPath;
    if (outputPath.empty()) {
        outputPath = _metadata.info.fileName;
    }

    _pieceManager =
        std::make_shared<PieceManager>(_metadata, cv, std::move(p), outputPath, _completionMutex);
    _peerManager = std::make_unique<PeerManager>(peers, _metadata.infoHash, peerId);

    _peerManager->start(_pieceManager);

    std::unique_lock<std::mutex> lock(_completionMutex);
    cv.wait(lock, [&] { return _pieceManager->isComplete(); });

    lock.unlock();
    _peerManager->stop();
    _pieceManager.reset();

    if (!_expectedSha256.empty()) {
        if (!_logging) {
            std::printf("\033[1;33m\nVerifying file integrity...\033[0m\n");
            std::fflush(stdout);
        }

        const std::filesystem::path finalFile = outputPath;
        const std::string actualHash = bt::utils::ByteReader::sha256sum(finalFile);

        if (actualHash.size() != _expectedSha256.size() ||
            !std::equal(_expectedSha256.begin(), _expectedSha256.end(), actualHash.begin(),
                        [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            throw std::runtime_error("SHA256 mismatch for output file " + finalFile.string() +
                                     ": expected " + _expectedSha256 + ", actual " + actualHash);
        }

        if (_logging) {
            spdlog::info("SHA256 verification passed: {}", actualHash);
            spdlog::info("Download complete. Output file: {}", finalFile.string());
        } else {
            std::printf("\033[1;32mSHA256 verification passed!\033[0m\n");
            std::printf("\033[1;36mOutput file:\033[0m %s\n\n", finalFile.string().c_str());
        }
    }
}