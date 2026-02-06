#include "app/torrent_orchestrator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "core/tracker_communicator.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <unistd.h>

using namespace bt;

TorrentOrchestrator::TorrentOrchestrator(std::string path)
    : _metadata(core::parseTorrentData(path)) {};

void TorrentOrchestrator::start() {
    // TODO: Move to peer manager
    const auto peerId = core::generateId(20);
    spdlog::debug("Generated peer id: %s", peerId);
    const auto trackerResponse = core::announceAndGetPeers(_metadata, peerId);

    auto pieceManager = std::make_shared<PieceManager>(_metadata);

    // peer blob gets consumed when creating the peer manager
    _peerManager =
        std::make_unique<PeerManager>(trackerResponse.peersBlob, _metadata.infoHash, peerId);
    _peerManager->start(pieceManager);
    spdlog::info("Peermanager successfully started.");
    sleep(20);
    _peerManager->stop();
}