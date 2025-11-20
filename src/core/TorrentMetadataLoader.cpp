#include <spdlog/spdlog.h>
#include <string>

#include "BencodeParser.hpp"
#include "TorrentMetadataLoader.hpp"

namespace bt::core {
TorrentMetadata parseTorrentData(std::string_view path) {
    using namespace detail;
    const auto torrentData = loadTorrentFile(path);

    spdlog::debug("Parsing torrent data of size: {} bytes", torrentData.size());

    const auto rootDict = parseRootDict(torrentData);
    const auto metadata = parseRootMetadata(rootDict);

    debugLogTorrentMetadata(metadata);
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

    const auto fileLength = bencode::extractValueFromDict<int64_t>(infoDict, DictKeys::LENGTH);
    if (fileLength < 0)
        throw std::runtime_error("Invalid file length in torrent metadata");

    const auto fileName = bencode::extractValueFromDict<std::string>(infoDict, DictKeys::NAME);

    return TorrentMetadata::Info{.pieceHashes = pieceHashes,
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
    metadata.info = parseInfoDict(infoDict);

    return metadata;
}

std::vector<Sha1Hash> parsePieceHashes(const std::string& piecesStr) {
    if (piecesStr.size() % HASH_LENGTH != 0) {
        throw std::runtime_error("Invalid pieces string length in torrent metadata");
    }

    size_t numHashes = piecesStr.size() / HASH_LENGTH;
    std::vector<Sha1Hash> pieceHashes;
    pieceHashes.reserve(numHashes);

    for (size_t i = 0; i < numHashes; ++i) {
        Sha1Hash hash;
        std::copy_n(reinterpret_cast<const uint8_t*>(piecesStr.data()) + i * HASH_LENGTH,
                    HASH_LENGTH, hash.begin());
        pieceHashes.push_back(hash);
    }

    spdlog::debug("Parsed {} piece hashes from torrent metadata", pieceHashes.size());
    return pieceHashes;
}

Sha1Hash calculateInfoHash(const TorrentMetadata::Info& infoDictData) {
    // Placeholder implementation - actual SHA-1 calculation would go here
    Sha1Hash dummyHash = {};
    return dummyHash;
}

std::string loadTorrentFile(std::string_view& path) {
    spdlog::info("Loading torrent file from path: {}", path);

    // get a file descriptor
    FILE* file = fopen(std::string(path).c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Failed to open torrent file");
    }

    // seek to end to get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // read file contents
    std::vector<uint8_t> fileData(fileSize);
    size_t bytesRead = fread(fileData.data(), 1, fileSize, file);
    if (bytesRead != static_cast<size_t>(fileSize)) {
        spdlog::error("Failed to read entire torrent file: {}", path);
        fclose(file);
        throw std::runtime_error("Failed to read entire torrent file");
    }
    fclose(file);

    spdlog::debug("Successfully loaded torrent file: {} ({} bytes)", path, fileSize);

    return std::string(reinterpret_cast<const char*>(fileData.data()), fileData.size());
}

void debugLogTorrentMetadata(const TorrentMetadata& metadata) {
    spdlog::debug("Torrent Metadata:");
    spdlog::debug("  Announce URL: {}", metadata.announce);
    spdlog::debug("  Comment: {}", metadata.comment);
    spdlog::debug("  Creation Date: {}", metadata.creationDate);
    spdlog::debug("  Info:");
    spdlog::debug("    File Name: {}", metadata.info.fileName);
    spdlog::debug("    File Length: {}", metadata.info.fileLength);
    spdlog::debug("    Piece Length: {}", metadata.info.pieceLength);
    spdlog::debug("    Number of Pieces: {}", metadata.info.pieceHashes.size());
}
} // namespace detail
} // namespace bt::core