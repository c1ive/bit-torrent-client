#pragma once
#include <array>
#include <cstdint>
namespace bt {

typedef std::array<uint8_t, 4> IPAddress;

/**
 * @brief Represents a remote peer (network endpoint) in the BitTorrent client.
 *
 * Encapsulates the information required to identify and connect to another client:
 * an IP address and a TCP port number.
 *
 * Members:
 * - ip:   The peer's IP address. Accepts IPv4 or IPv6 as represented by the IPAddress type.
 * - port: The peer's TCP port number (0-65535). Zero typically indicates an unspecified port.
 *
 * @note The semantics of IPAddress (ownership, mutability, serialization) are defined elsewhere.
 * @warning Ensure port is validated before attempting network operations.
 */
struct Peer {
    IPAddress ip;
    uint16_t port;
};
} // namespace bt
