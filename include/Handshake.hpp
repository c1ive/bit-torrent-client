#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace bt {

/**
 * @brief BitTorrent handshake message structure.
 *
 * Represents the essential fields of a BitTorrent handshake message used
 * during the initial connection between peers. The handshake consists of
 * a protocol identifier string, a 20-byte info hash, and a 20-byte peer ID.
 *
 * Members:
 * - pstr
 *   The protocol string identifying the BitTorrent protocol. Standard value
 *   is "BitTorrent protocol".
 *
 * - infoHash
 *   The 20-byte SHA-1 hash of the torrent's "info" dictionary. This uniquely
 *   identifies the torrent being shared.
 *
 * - peerId
 *   A 20-byte identifier for the connecting peer. This is typically a
 *   client-specific string used to identify the client software and version.
 *
 * Notes:
 * - All byte arrays are stored as std::array<uint8_t, 20> for fixed-size
 *   representation.
 * - The handshake message is sent immediately upon establishing a TCP
 *   connection with another peer.
 */
struct Handshake {
    std::string pstr = "BitTorrent protocol"; // Protocol string
    std::array<uint8_t, 20> infoHash;         // 20-byte info hash
    std::array<uint8_t, 20> peerId;           // 20-byte
};
} // namespace bt
