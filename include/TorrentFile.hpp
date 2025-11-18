#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bt {
/**
 * @brief Metadata representation of a parsed .torrent file.
 *
 * This struct contains the essential metadata a BitTorrent client needs to
 * identify a torrent and verify/download its pieces.
 *
 * Members:
 * - announce
 *   The tracker URL taken from the torrent's "announce" field. May be an
 *   empty string if the torrent does not specify a tracker (e.g. DHT-only).
 *
 * - infoHash
 *   The 20-byte SHA-1 hash of the bencoded "info" dictionary (raw binary, not
 *   hex). This value uniquely identifies the torrent on the network and is
 *   used when communicating with trackers and peers.
 *
 * - pieceHashes
 *   A vector of 20-byte SHA-1 hashes (raw binary) listing the hash for each
 *   piece in order. The number of entries should match the number of pieces
 *   implied by fileLength and pieceLength.
 *
 * - pieceLength
 *   The length in bytes of each piece as specified in the torrent metadata.
 *   All pieces except possibly the last one are expected to be this size.
 *
 * - fileLength
 *   Total length in bytes of the file described by this torrent (for
 *   single-file torrents). For multi-file torrents, this field should be set
 *   according to how the client models multi-file layouts; this struct is
 *   primarily suited for single-file torrents.
 *
 * - fileName
 *   The filename from the torrent's "info" dictionary (for single-file
 *   torrents). For multi-file torrents this may represent the top-level
 *   directory name or be unused depending on client design.
 *
 * Notes:
 * - All hashes are stored as raw 20-byte binary arrays (std::array<uint8_t,20>),
 *   not as hex strings.
 * - Consistency expectation: pieceHashes.size() == ceil(fileLength / pieceLength).
 * - Clients should validate pieceLength > 0 and that the number of piece hashes
 *   matches the expected piece count before attempting downloads.
 */
constexpr int HASH_LENGTH = 20;

struct TorrentFile {
    std::string announce;
    std::array<uint8_t, HASH_LENGTH> infoHash;
    std::vector<std::array<uint8_t, HASH_LENGTH>> pieceHashes;
    uint64_t pieceLength;
    uint64_t fileLength;
    std::string fileName;
};


const TorrentFile constructTorrentFile( std::string_view& path );

namespace detail {
const std::vector<uint8_t> loadTorrentFile( std::string_view& path);
const std::array<uint8_t, HASH_LENGTH> calculateInfoHash( const std::vector<uint8_t>& torrentBuffer );
}
} // namespace btd