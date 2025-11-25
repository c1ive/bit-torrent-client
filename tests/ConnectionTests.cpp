#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "TorrentMetadataLoader.hpp"
#include "TrackerCommunicator.hpp"

#include <filesystem>
#include <string>

namespace {
std::filesystem::path fixtureTorrentPath() {
    static const auto path =
        std::filesystem::path(__FILE__).parent_path() / "debian-13.2.0-arm64-netinst.iso.torrent";
    return path;
}
} // namespace

TEST_CASE("generateId produces correct length and charset") {
    constexpr int len = 20;
    const auto id = bt::core::detail::generateId(len);
    REQUIRE(id.size() == static_cast<size_t>(len));

    // Allowed characters set used in implementation
    static const std::string allowed =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (char c : id) {
        REQUIRE_MESSAGE(allowed.find(c) != std::string::npos,
                        "Unexpected character in generated id: " << c);
    }
}

TEST_CASE("buildTrackerUrl includes required parameters") {
    const auto torrentPath = fixtureTorrentPath();
    REQUIRE_MESSAGE(std::filesystem::exists(torrentPath),
                    "Fixture torrent missing: " << torrentPath.string());

    const auto metadata = bt::core::parseTorrentData(torrentPath.string());
    const auto peerId = bt::core::detail::generateId(20);

    const auto trackerUrl = bt::core::detail::buildTrackerUrl(metadata, peerId);

    // Boost URL stores encoded URL in its buffer
    const std::string urlStr = std::string(trackerUrl.buffer());

    CHECK(urlStr.find("info_hash=") != std::string::npos);
    CHECK(urlStr.find("peer_id=") != std::string::npos);
    CHECK(urlStr.find(peerId) != std::string::npos);
    CHECK(urlStr.find("left=" + std::to_string(metadata.info.fileLength)) != std::string::npos);
    CHECK(urlStr.find("compact=1") != std::string::npos);
    CHECK(urlStr.find("uploaded=0") != std::string::npos);
    CHECK(urlStr.find("downloaded=0") != std::string::npos);
}

TEST_CASE("announce throws on non-200 response") {
    // Using a known 404 endpoint to force failure
    CHECK_THROWS_AS(bt::core::detail::announceToTracker("https://httpbin.org/status/404"),
                    std::runtime_error);
}
