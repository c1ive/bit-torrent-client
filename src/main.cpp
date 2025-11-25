#include "core/torrent_metadata_loader.hpp"
#include "core/tracker_communicator.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include <argparse/argparse.hpp>
#include <string_view>

using namespace bt;
int main(int argc, char* argv[]) {
    // Parse command line arguments (if any)
    argparse::ArgumentParser app("bit-torrent-client");
    app.add_argument("-t", "--torrent").required().help("Path to the torrent file");
    app.add_argument("-v", "--verbose")
        .help("Verbose logs (for debugging)")
        .default_value(false)
        .implicit_value(true);

    try {
        app.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << app;
        return 1;
    }

    if (app["--verbose"] == true) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    const auto path = app.get<std::string>("--torrent");

    // Logger setup
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");

    /// Setup and start app orchestrators
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