#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"

#include <algorithm>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstddef>
#include <cstdint>
#include <spdlog/spdlog.h>

using namespace asio::ip;
using namespace asio;

namespace bt::core {
HandshakeMsg serializeHandshake(const core::Sha1Hash& infoHash, std::string_view peerId) {
    HandshakePacket pkg{};

    pkg.pstrlen = msg::LEN;

    // Safety check for protocol string length
    size_t copy_len = std::min(sizeof(pkg.pstr), std::strlen(msg::PROTOCOL));
    std::copy_n(msg::PROTOCOL, copy_len, pkg.pstr);

    std::copy_n(infoHash.data(), 20, pkg.info_hash);
    std::copy_n(peerId.data(), 20, pkg.peer_id);

    HandshakeMsg result_buffer;
    static_assert(sizeof(HandshakePacket) == 68, "Handshake packet structure size is incorrect!");

    std::copy_n(reinterpret_cast<const uint8_t*>(&pkg), sizeof(HandshakePacket),
                result_buffer.data());
    return result_buffer;
}

bool verifyHandshake(const HandshakeMsg& handshakeResponse, const Sha1Hash& expectedInfoHash) {
    // 1. Check Protocol Length
    if (handshakeResponse[0] != msg::LEN) {
        return false;
    }

    // 2. Check Protocol String
    auto proto_start = handshakeResponse.begin() + 1;
    auto proto_end = proto_start + msg::LEN;

    if (!std::equal(proto_start, proto_end, msg::PROTOCOL)) {
        return false;
    };

    // 3. Check Info Hash
    // Offset: 1 (len) + 19 (proto) + 8 (reserved) = 28
    const size_t info_hash_offset = 28;
    auto hash_start = handshakeResponse.begin() + info_hash_offset;

    // Compare received hash against the one we expect
    if (!std::equal(hash_start, hash_start + 20, expectedInfoHash.data())) {
        return false;
    }

    return true;
}
} // namespace bt::core