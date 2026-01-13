#pragma once

#include "core/torrent_metadata_loader.hpp"
#include <array>
#include <asio/io_context.hpp>
#include <cstddef>
#include <cstdint>

#include <asio.hpp>
#include <format>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace bt::core {
typedef std::array<uint8_t, 4> IpAddr;
typedef std::array<uint8_t, 68> HandshakeMsg;

namespace msg {
constexpr uint8_t LEN = 0x13;
constexpr const char* PROTOCOL = "BitTorrent protocol";
constexpr uint64_t RESERVED = 0;
constexpr size_t HANDSHAKE_LEN = 68;
} // namespace msg

#pragma pack(push, 1)
struct HandshakePacket {
    uint8_t pstrlen;
    char pstr[19];
    uint8_t reserved[8];
    uint8_t info_hash[20];
    char peer_id[20];
};
#pragma pack(pop)

struct Peer {
    uint16_t port;
    IpAddr ip;

    std::string getIpStr() const {
        const auto& f = [this](int x) { return ip[x]; };
        auto res = std::format("{}.{}.{}.{}", f(0), f(1), f(2), f(3));
        return res;
    }
};

HandshakeMsg serializeHandshake(const Sha1Hash& infoHash, std::string_view peerId);
bool verifyHandshake(const HandshakeMsg& handshakeResponse, const Sha1Hash& expectedInfoHash);
}; // namespace bt::core