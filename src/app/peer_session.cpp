#include "app/peer_session.hpp"
#include "core/peer_communicator.hpp"
#include <asio/awaitable.hpp>
#include <cstdint>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace bt {

PeerSession::PeerSession(asio::io_context& io_context) : _socket(io_context) {}

asio::awaitable<uint32_t> PeerSession::_readMsgLen() {
    uint32_t network_len = 0;
    auto [ec, _] =
        co_await asio::async_read(_socket, asio::buffer(&network_len, sizeof(network_len)),
                                  asio::as_tuple(asio::use_awaitable));

    if (ec) {
        _setState(PeerState::ERROR);
        throw std::runtime_error{ec.message()};
    }

    uint32_t len = ntohl(network_len);
    co_return len;
}

void PeerSession::_handleMessage(core::msg::id msg_id, std::span<uint8_t> payload) {
    using namespace core::msg;
    switch (msg_id) {
    case id::CHOKE:
        spdlog::debug("Peer choked us");
        _peer_choking = true;
        break;
    case id::UNCHOKE:
        spdlog::info("Peer unchoked us! We can request now.");
        _peer_choking = false;
        // Trigger request logic here
        break;
    case id::HAVE:
        // Parse payload (4 bytes index) and update bitfield
        break;
    case id::BITFIELD:
        spdlog::info("Received Bitfield of size {}", payload.size());
        // Parse bitfield logic here
        // Usually, you immediately send 'Interested' after processing this
        break;
    case id::PIECE:
        // Handle incoming data block
        break;
    default:
        spdlog::debug("Received unknown or unhandled message ID: {}", static_cast<uint8_t>(msg_id));
        break;
    }
}

asio::awaitable<void> PeerSession::run() {
    _setState(PeerState::BITFIELD_WAIT);
    while (_socket.is_open() && _state != PeerState::ERROR) {
        uint32_t len = 0;

        try {
            len = co_await _readMsgLen();
        } catch (std::runtime_error& e) {
            spdlog::error("Connection lost reading length: {}", e.what());
            co_return;
        }

        if (len == 0) {
            spdlog::debug("Received Keep-Alive");
            continue; // Go back to start of loop
        }
        std::vector<uint8_t> message_buffer(len);
        auto [ec2, bytes_read] = co_await asio::async_read(_socket, asio::buffer(message_buffer),
                                                           asio::as_tuple(asio::use_awaitable));

        if (ec2) {
            spdlog::error("Connection lost reading payload: {}", ec2.message());
            _state = PeerState::ERROR;
            co_return;
        }

        core::msg::id msg_id = static_cast<core::msg::id>(message_buffer[0]);
        std::span<uint8_t> payload = {};
        if (len > 1) {
            payload = std::span<uint8_t>(message_buffer.data() + 1, len - 1);
        }

        _handleMessage(msg_id, payload);
    }
};

asio::awaitable<void> PeerSession::connect(const core::Peer& peer) {
    _setState(PeerState::CONNECTING);
    asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(peer.getIpStr()), peer.port);
    spdlog::debug("Connecting to peer at {}:{}", peer.getIpStr(), peer.port);

    auto [ec] = co_await _socket.async_connect(endpoint, asio::as_tuple(asio::use_awaitable));

    if (ec) {
        spdlog::debug("Failed to connect to peer at {}:{} - {}", peer.getIpStr(), peer.port,
                      ec.message());
        _state = PeerState::ERROR;
        co_return;
    }
    spdlog::debug("Successfully connected to peer at {}:{}", peer.getIpStr(), peer.port);
}

asio::awaitable<void> PeerSession::doHandshake(const core::Sha1Hash& infoHash,
                                               std::string_view peerId) {
    _setState(PeerState::HANDSHAKING);
    spdlog::debug("Performing handshake...");
    core::HandshakeMsg handshake = core::serializeHandshake(infoHash, peerId);

    spdlog::debug("Sending buffer to peer.");
    auto [ec1, len] = co_await asio::async_write(_socket, asio::buffer(handshake),
                                                 asio::as_tuple(asio::use_awaitable));
    if (ec1) {
        spdlog::error("Sending handshake failed: {}", ec1.message());
        _state = PeerState::ERROR;
        co_return;
    }
    if (len != handshake.size()) {
        spdlog::error("Failed to send whole handshake buffer to peer");
        _state = PeerState::ERROR;
        co_return;
    }

    spdlog::debug("Handshake sent, waiting for response...");
    std::array<uint8_t, core::msg::HANDSHAKE_LEN> handshakeResponse{};

    auto [ec2, bytes_transferred] = co_await asio::async_read(
        _socket, asio::buffer(handshakeResponse), asio::as_tuple(asio::use_awaitable));

    spdlog::debug("Read {} bytes from peer", bytes_transferred);
    if (ec2) {
        spdlog::debug("Failed to read handshake: {}", ec2.message());
        _state = PeerState::ERROR;
        co_return;
    }

    if (!core::verifyHandshake(handshakeResponse, infoHash)) {
        // Its verry common that a handshake with a peer fails, so its only logged in debug mode
        spdlog::debug("Handshake verification failed.");
        _state = PeerState::ERROR;
        co_return;
    }

    spdlog::info("Handshake successfully completed with {}:{}.",
                 _socket.remote_endpoint().address().to_string(), _socket.remote_endpoint().port());
}
} // namespace bt