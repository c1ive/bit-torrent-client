#include "app/torrent_orchestrator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "core/tracker_communicator.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

using namespace bt;

TorrentOrchestrator::TorrentOrchestrator(std::string path)
    : _metadata(core::parseTorrentData(path)) {};

void TorrentOrchestrator::download() {
    // TODO: Move to peer manager
    const auto peerId = core::generateId(20);
    spdlog::debug("Generated peer id: %s", peerId);
    const auto trackerResponse = core::announceAndGetPeers(_metadata, peerId);
    auto peers = trackerResponse.peersBlob;

    _pieceManager = std::make_shared<PieceManager>(_metadata, cv);
    _peerManager = std::make_unique<PeerManager>(peers, _metadata.infoHash, peerId);

    _peerManager->start(_pieceManager);

    std::unique_lock<std::mutex> lock(_completionMutex);
    cv.wait(lock, [&] { return _pieceManager->isComplete(); });

    _peerManager->stop();
}