#include "core/torrent_metadata_loader.hpp"
#include "core/bencode_parser.hpp"

#include <algorithm>
#include <boost/uuid/detail/sha1.hpp>
#include <chrono>
#include <fstream>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#include <string>

namespace bt::core {
TorrentMetadata parseTorrentData(std::string_view path) {
    const auto torrentData = detail::loadTorrentFile(path);

    spdlog::debug("Parsing torrent data of size: {} bytes", torrentData.size());

    // Start spdlog timer
    auto startTime = std::chrono::high_resolution_clock::now();

    const auto& torrentString = torrentData;
    const auto rootDict = detail::parseRootDict(torrentString);
    const auto metadata = detail::parseRootMetadata(rootDict);

    // End spdlog timer
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    spdlog::debug("Parsed and loaded torrent metadata in {} us", duration);
    detail::debugLogTorrentMetadata(metadata);
    return metadata;
}

namespace detail {
// Parse the raw torrent data and return the root dictionary
bencode::Dict parseRootDict(const std::string& torrentData) {
    constexpr size_t MAX_TORRENT_SIZE = 10 * 1024 * 1024; // 10 MB
    if (torrentData.size() > MAX_TORRENT_SIZE) {
        spdlog::error("Torrent file too large: {} bytes", torrentData.size());
        throw std::runtime_error("Torrent file exceeds maximum allowed size");
    }

    const auto value = bencode::parse(torrentData);
    if (!std::holds_alternative<bencode::Dict>(value)) {
        spdlog::error("Torrent file root is not a dictionary");
        throw std::runtime_error("Invalid torrent file format");
    }

    return std::get<bencode::Dict>(value);
}

// Parse the "info" dictionary into TorrentMetadata::Info
TorrentMetadata::Info parseInfoDict(const bencode::Dict& infoDict) {
    const auto pieceLength =
        bencode::extractValueFromDict<int64_t>(infoDict, DictKeys::PIECE_LENGTH);
    if (pieceLength <= 0)
        throw std::runtime_error("Invalid piece length in torrent metadata");

    const auto piecesStr = bencode::extractValueFromDict<std::string>(infoDict, DictKeys::PIECES);
    const auto pieceHashes = parsePieceHashes(piecesStr);

    // DATA INTEGRITY
    // Ensure we didn't lose or gain data during the vector conversion.
    // The raw string size must match the vector size * hash length exactly.
    assert(piecesStr.size() == pieceHashes.size() * HASH_LENGTH);

    const auto fileLength = bencode::extractValueFromDict<int64_t>(infoDict, DictKeys::LENGTH);
    if (fileLength < 0)
        throw std::runtime_error("Invalid file length in torrent metadata");

    const auto fileName = bencode::extractValueFromDict<std::string>(infoDict, DictKeys::NAME);

    return TorrentMetadata::Info{.pieceHashes = pieceHashes,
                                 .rawPieces = piecesStr,
                                 .pieceLength = static_cast<uint64_t>(pieceLength),
                                 .fileLength = static_cast<uint64_t>(fileLength),
                                 .fileName = fileName};
}

// Parse the root metadata (excluding "info") and assemble TorrentMetadata
TorrentMetadata parseRootMetadata(const bencode::Dict& rootDict) {
    TorrentMetadata metadata;
    metadata.comment = bencode::extractValueFromDict<std::string>(rootDict, DictKeys::COMMENT);
    metadata.announce = bencode::extractValueFromDict<std::string>(rootDict, DictKeys::ANNOUNCE);
    metadata.creationDate =
        bencode::extractValueFromDict<int64_t>(rootDict, DictKeys::CREATION_DATE);

    const auto& infoDict = bencode::extractValueFromDict<bencode::Dict>(rootDict, DictKeys::INFO);
    const auto& infoDictData = parseInfoDict(infoDict);
    metadata.info = infoDictData;

    metadata.infoHash = calculateInfoHash(infoDictData);

    return metadata;
}

std::vector<Sha1Hash> parsePieceHashes(const std::string& piecesStr) {
    static_assert(sizeof(Sha1Hash) == HASH_LENGTH, "Sha1Hash size mismatch");

    if (piecesStr.size() % HASH_LENGTH != 0) {
        throw std::runtime_error("Invalid pieces string length in torrent metadata");
    }

    size_t numHashes = piecesStr.size() / HASH_LENGTH;
    std::vector<Sha1Hash> pieceHashes;
    pieceHashes.resize(numHashes);

    // Get raw pointers
    const uint8_t* src = reinterpret_cast<const uint8_t*>(piecesStr.data());
    uint8_t* dst = reinterpret_cast<uint8_t*>(pieceHashes.data());

    // Before performing a raw memory copy, strictly verify that the
    // destination buffer size in bytes matches the source size.
    // This protects against future logic errors in 'resize' or 'numHashes' calculation.
    assert(pieceHashes.size() * sizeof(Sha1Hash) == piecesStr.size());
    std::memcpy(dst, src, piecesStr.size());

    return pieceHashes;
}

Sha1Hash calculateInfoHash(const TorrentMetadata::Info& infoDictData) {
    bencode::Dict infoDict;
    infoDict.values[DictKeys::PIECE_LENGTH] = static_cast<int64_t>(infoDictData.pieceLength);
    infoDict.values[DictKeys::LENGTH] = static_cast<int64_t>(infoDictData.fileLength);
    infoDict.values[DictKeys::NAME] = infoDictData.fileName;
    infoDict.values[DictKeys::PIECES] = infoDictData.rawPieces;

    const auto& encodedInfo = bencode::encode(infoDict);

    boost::uuids::detail::sha1 sha1;
    boost::uuids::detail::sha1::digest_type hash;
    sha1.process_bytes(encodedInfo.data(), encodedInfo.size());
    sha1.get_digest(hash);

    Sha1Hash infoHash;
    std::copy_n(hash, 20, infoHash.begin());

    return infoHash;
}

std::string loadTorrentFile(const std::filesystem::path& path) {
    spdlog::info("Loading torrent file from path: {}", path.string());

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Torrent file does not exist: " + path.string());
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open torrent file: " + path.string());
    }

    auto fileSize = file.tellg();
    if (fileSize < 0) {
        throw std::runtime_error("Failed to determine file size: " + path.string());
    }

    file.seekg(0, std::ios::beg);
    std::string fileData;
    fileData.resize(static_cast<size_t>(fileSize)); // Allocate once

    // Read directly into string buffer
    if (!file.read(fileData.data(), static_cast<std::streamsize>(fileSize))) {
        throw std::runtime_error("Failed to read...");
    }

    // Verify that the file stream actually read the number of bytes we expected.
    // file.read() sets failbit on error, but gcount() confirms the byte count.
    assert(file.gcount() == static_cast<std::streamsize>(fileSize));
    return fileData;
}

void debugLogTorrentMetadata(const TorrentMetadata& metadata) {
    spdlog::debug("Torrent Metadata:");
    spdlog::debug("  Announce URL: {}", metadata.announce);
    spdlog::debug("  Comment: {}", metadata.comment);
    spdlog::debug("  Creation Date: {}", metadata.creationDate);
    spdlog::debug("  Info Hash: {:spn}", spdlog::to_hex(metadata.infoHash));
    spdlog::debug("  Info:");
    spdlog::debug("    File Name: {}", metadata.info.fileName);
    spdlog::debug("    File Length: {}", metadata.info.fileLength);
    spdlog::debug("    Piece Length: {}", metadata.info.pieceLength);
    spdlog::debug("    Number of Pieces: {}", metadata.info.pieceHashes.size());
}
} // namespace detail
} // namespace bt::core