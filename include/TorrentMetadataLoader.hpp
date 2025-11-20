#pragma once
#include "BencodeParser.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

/**
 * @file TorrentMetadataLoader.hpp
 * @brief Utilities for loading and parsing .torrent metadata.
 *
 * Declares types and functions to parse bencoded torrent files, extract
 * metadata, and compute SHA-1 info-hash.
 */

namespace bt::core {

constexpr int HASH_LENGTH = 20;
using Sha1Hash = std::array<uint8_t, HASH_LENGTH>;

/** Keys used in bencoded dictionaries. */
struct DictKeys {
    static constexpr const char* COMMENT = "comment";
    static constexpr const char* ANNOUNCE = "announce";
    static constexpr const char* INFO = "info";
    static constexpr const char* PIECE_LENGTH = "piece length";
    static constexpr const char* LENGTH = "length";
    static constexpr const char* NAME = "name";
    static constexpr const char* PIECES = "pieces";
    static constexpr const char* CREATION_DATE = "creation date";
};

/** Subset of the torrent's "info" dictionary (per-piece and file info). */
struct TorrentMetadata {
    std::string announce;
    std::string comment;
    uint64_t creationDate;
    Sha1Hash infoHash;

    struct Info {
        std::vector<Sha1Hash> pieceHashes;
        uint64_t pieceLength;
        uint64_t fileLength;
        std::string fileName;
    };

    Info info;
};

/** Parse a .torrent file buffer into TorrentMetadata. */
TorrentMetadata parseTorrentData(std::string_view torrentData);

namespace detail {
std::string loadTorrentFile(std::string_view& path);
bencode::Dict parseRootDict(const std::string& torrentData);
TorrentMetadata::Info parseInfoDict(const bencode::Dict& infoDict);
TorrentMetadata parseRootMetadata(const bencode::Dict& rootDict);
Sha1Hash calculateInfoHash(const TorrentMetadata::Info& infoDictData);
std::vector<Sha1Hash> parsePieceHashes(const std::string& piecesStr);

void debugLogTorrentMetadata(const TorrentMetadata& metadata);
} // namespace detail
} // namespace bt::core
