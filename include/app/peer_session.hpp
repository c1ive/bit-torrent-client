#pragma once

#include <core/peer_communicator.hpp>

#include <asio.hpp>

namespace bt {
enum class PeerState { HANDSHAKE_COMPLETE, ERROR };
class PeerSession {
public:
    explicit PeerSession(asio::io_context& io_context);

    inline PeerState getState() {
        return _state;
    };
    asio::awaitable<void> connect(const core::Peer& peer);

    asio::awaitable<void> doHandshake(const core::Sha1Hash& infoHash, std::string_view peerId);
    asio::awaitable<void> run();

    // Helper to check connection status
    // bool isOpen() const {
    //     return _socket.is_open();
    // }

private:
    PeerState _state;
    asio::ip::tcp::socket _socket;

    asio::awaitable<uint32_t> _readMsgLen();
    void _handleMessage(core::msg::id msg_id, std::span<uint8_t> payload);
};
} // namespace bt