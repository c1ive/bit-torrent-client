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

enum class id : uint8_t {
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8
};
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

class ByteWriter {
public:
    void write_u32(uint32_t val);
    void write_u8(uint8_t val);
    const std::vector<uint8_t>& data() const;

private:
    std::vector<uint8_t> _data;
};

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

// void readMessage( std::span<typename Type, size_t Extent> )
}; // namespace bt::core