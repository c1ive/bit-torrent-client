#include "TorrentMetadataLoader.hpp"
#include "TrackerCommunicator.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include <string_view>

using namespace bt;
int main(int argc, char* argv[]) {
    // Parse command line arguments (if any)
    if (argc < 2) {
        spdlog::error("Usage: {} <path_to_torrent_file>", argv[0]);
        return 1;
    }
    std::string_view path = argv[1];

    // Logger setup
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");

    spdlog::info("Main loop starts now");
    core::TorrentMetadata torrent;
    try {
        torrent = core::parseTorrentData(path);
        spdlog::info("Torrent metadata loaded successfully.");
    } catch (const std::exception& e) {
        spdlog::critical("Error constructing torrent file: {}", e.what());
    }

    const auto trackerResponse = core::announceAndGetPeers(torrent);

    spdlog::info("Tracker response: interval: {}, number of peers: {}", trackerResponse.interval,
                 trackerResponse.peersBlob.size());

    spdlog::info("Exiting.");
    return 0;
}