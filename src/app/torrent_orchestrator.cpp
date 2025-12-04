#include "app/torrent_orchestrator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "core/tracker_communicator.hpp"
#include <spdlog/spdlog.h>

using namespace bt;

TorrentOrchestrator::TorrentOrchestrator(std::string path)
    : _metadata(core::parseTorrentData(path)) {};

void TorrentOrchestrator::start() {
    // For now just call the core function, should be done by a sepearte manager in the future to
    // handle the interval...
    const auto trackerResponse = core::announceAndGetPeers(_metadata);

    // peer blob gets consumed when creating the peer manager
    _peerManager.emplace(std::move(trackerResponse.peersBlob));
    _peerManager->start();
}