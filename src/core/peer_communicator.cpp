#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>

using namespace asio::ip;
using namespace asio;

namespace bt::core {

PeerSession::PeerSession(asio::io_context& io_context) : _socket(io_context) {}

bool PeerSession::connect(const Peer& peer) {
    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(peer.getIpStr()), peer.port);
    spdlog::debug("Connecting to peer at {}:{}", peer.getIpStr(), peer.port);

    try {
        _socket.connect(endpoint);
        spdlog::debug("Successfully connected to peer at {}:{}", peer.getIpStr(), peer.port);
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to peer at {}:{} - {}", peer.getIpStr(), peer.port,
                      e.what());
        return false;
    }

    return true;
}

void PeerSession::doHandshake(core::Sha1Hash& infoHash, std::string_view peerId) {
    spdlog::debug("Performing handshake...");
    HandshakeMsg handshake = _serializeHandshake(infoHash, peerId);

    spdlog::debug("Sending buffer to peer.");
    _socket.send(asio::buffer(handshake));

    std::array<uint8_t, 256> answer{};
    _socket.read_some(asio::buffer(answer));
    spdlog::debug("Read {} bytes from peer", answer.size());

    spdlog::debug("Handshake completed.");
}

HandshakeMsg PeerSession::_serializeHandshake(core::Sha1Hash& infoHash, std::string_view peerId) {
    if (infoHash.size() != 20 || peerId.size() != 20) {
        throw std::invalid_argument("InfoHash and PeerID must be exactly 20 bytes");
    }

    HandshakePacket pkg{};

    pkg.pstrlen = msg::LEN;

    // Safety check for protocol string length
    size_t copy_len = std::min(sizeof(pkg.pstr), std::strlen(msg::PROTOCOL));
    std::memcpy(pkg.pstr, msg::PROTOCOL, copy_len);

    std::memcpy(pkg.info_hash, infoHash.data(), 20);
    std::memcpy(pkg.peer_id, peerId.data(), 20);

    HandshakeMsg result_buffer;
    static_assert(sizeof(HandshakePacket) == 68, "Handshake packet structure size is incorrect!");

    std::memcpy(result_buffer.data(), &pkg, sizeof(HandshakePacket));
    return result_buffer;
}
} // namespace bt::core