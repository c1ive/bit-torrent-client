#include "app/torrent_orchestrator.hpp"

#include <argparse/argparse.hpp>
#include <ostream>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string_view>

struct Settings {
    std::string torrent_path;
    bool verbose;
};

static Settings parse_args(int argc, char* argv[]) {
    argparse::ArgumentParser app("bit-torrent-client");

    app.add_argument("-t", "--torrent").required().help("Path to the torrent file");
    app.add_argument("-v", "--verbose")
        .help("Verbose logs")
        .default_value(false)
        .implicit_value(true);
    try {
        app.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << app;
        std::exit(1);
    }

    return {app.get<std::string>("--torrent"), app.get<bool>("--verbose")};
}

static void init_logging(bool verbose) {
    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");
    spdlog::info("Logging initialized. Verbose mode: {}", verbose ? "ON" : "OFF");
}

using namespace bt;
int main(int argc, char* argv[]) {
    const auto settings = parse_args(argc, argv);
    init_logging(settings.verbose);

    try {
        spdlog::debug("Starting torrent orchestrator");
        TorrentOrchestrator to(settings.torrent_path);
        to.download();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}. Suggestion: re-run with -v for more details.", e.what());
        return 1;
    }

    return 0;
}