#include "app/peer_session.hpp"
#include "core/peer_communicator.hpp"
#include <asio/awaitable.hpp>
#include <cstdint>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace bt {

asio::awaitable<uint32_t> PeerSession::_readMsgLen() {
    uint32_t network_len = 0;
    auto [ec, _] =
        co_await asio::async_read(_socket, asio::buffer(&network_len, sizeof(network_len)),
                                  asio::as_tuple(asio::use_awaitable));

    if (ec) {

        _state = PeerState::ERROR;
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
        //_peer_choking = true;
        break;
    case id::UNCHOKE:
        spdlog::info("Peer unchoked us! We can request now.");
        //_peer_choking = false;
        // Trigger request logic here
        break;
    case id::HAVE:
        // Parse payload (4 bytes index) and update bitfield
        break;
    case id::BITFIELD:
        spdlog::debug("Received Bitfield of size {}", payload.size());
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
} // namespace bt