#include "app/torrent_orchestrator.hpp"
#include "core/tracker_communicator.hpp"
#include <spdlog/spdlog.h>
#include <string_view>

using namespace bt;

TorrentOrchestrator::TorrentOrchestrator(std::string path) : _metadata(_loadMetadata(path)) {};

core::TorrentMetadata TorrentOrchestrator::_loadMetadata(const std::string_view path) const {
    core::TorrentMetadata torrent;
    try {
        torrent = core::parseTorrentData(path);
        spdlog::info("Torrent metadata loaded successfully.");
    } catch (const std::exception& e) {
        spdlog::debug("Error constructing torrent file: {}", e.what());
        throw;
    };
    return torrent;
}

void TorrentOrchestrator::start() {
    // For now just call the core function, should be done by a sepearte manager in the future to
    // handle the interval...
    const auto trackerResponse = core::announceAndGetPeers(_metadata);
    spdlog::info("Tracker response: interval: {}, number of peers: {}", trackerResponse.interval,
                 trackerResponse.peersBlob.size());
}