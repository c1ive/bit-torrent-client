
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "TorrentMetadataLoader.hpp"

namespace {
std::filesystem::path fixtureTorrentPath() {
    static const auto path =
        std::filesystem::path(__FILE__).parent_path() / "debian-13.2.0-arm64-netinst.iso.torrent";
    return path;
}

uint8_t hexDigitToByte(char c) {
    if (c >= '0' && c <= '9')
        return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f')
        return static_cast<uint8_t>(10 + c - 'a');
    if (c >= 'A' && c <= 'F')
        return static_cast<uint8_t>(10 + c - 'A');
    throw std::invalid_argument("Invalid hex digit");
}

bt::core::Sha1Hash hexToSha1(std::string_view hex) {
    if (hex.size() != bt::core::HASH_LENGTH * 2) {
        throw std::invalid_argument("Unexpected info-hash length");
    }
    bt::core::Sha1Hash hash{};
    for (size_t i = 0; i < bt::core::HASH_LENGTH; ++i) {
        const auto high = hexDigitToByte(hex[i * 2]) << 4U;
        const auto low = hexDigitToByte(hex[i * 2 + 1]);
        hash[i] = static_cast<uint8_t>(high | low);
    }
    return hash;
}
} // namespace

TEST_CASE("parseTorrentData reads Debian sample torrent") {
    const auto torrentPath = fixtureTorrentPath();
    REQUIRE_MESSAGE(std::filesystem::exists(torrentPath),
                    "Fixture torrent missing: " << torrentPath.string());

    const auto metadata = bt::core::parseTorrentData(torrentPath.string());
    CHECK_FALSE(metadata.announce.empty());
    CHECK(metadata.creationDate > 0);
    CHECK(metadata.info.fileLength > 0);
    CHECK(metadata.info.pieceLength > 0);
    CHECK(metadata.info.fileName == "debian-13.2.0-arm64-netinst.iso");

    const auto expectedHash = hexToSha1("c24defaa5209b5730b09cc5c33f7b8b8e567c6b1");
    CHECK(metadata.infoHash == expectedHash);
    CHECK_FALSE(metadata.info.pieceHashes.empty());

    const auto expectedPieces =
        (metadata.info.fileLength + metadata.info.pieceLength - 1) / metadata.info.pieceLength;
    CHECK(metadata.info.pieceHashes.size() == expectedPieces);
}

TEST_CASE("PieceHashes rejects malformed input length") {
    std::string invalid(bt::core::HASH_LENGTH - 1, '\x00');
    CHECK_THROWS_AS(bt::core::detail::parsePieceHashes(invalid), std::runtime_error);
}

TEST_CASE("parsePieceHashes splits contiguous hashes correctly") {
    std::string raw(bt::core::HASH_LENGTH * 2, '\x00');
    raw[0] = '\x01';
    raw[bt::core::HASH_LENGTH] = '\xAA';

    const auto hashes = bt::core::detail::parsePieceHashes(raw);
    REQUIRE(hashes.size() == 2);
    CHECK(hashes[0][0] == 0x01);
    CHECK(hashes[1][0] == 0xAA);
}