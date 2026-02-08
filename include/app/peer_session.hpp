#pragma once

#include "app/piece_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <core/peer_communicator.hpp>

#include <asio.hpp>
#include <memory>
#include <vector>

namespace bt {
enum class PeerState {
    CONNECTING,
    HANDSHAKING,
    BITFIELD_WAIT, // Waiting for that optional initial bitfield
    READY,         // Handshake/Bitfield done, now participating in swarm
    DISCONNECTED,
    ERROR
};
class PeerSession {
public:
    explicit PeerSession(asio::io_context& io_context, std::shared_ptr<PieceManager> pieceManager);

    // inline PeerState getState() {
    //     return _state;
    // };
    asio::awaitable<void> connect(const core::Peer& peer);

    asio::awaitable<void> doHandshake(const core::Sha1Hash& infoHash, std::string_view peerId);
    asio::awaitable<void> run();

    // Helper to check connection status
    // bool isOpen() const {
    //     return _socket.is_open();
    // }

private:
    bool _am_choking = true;       // We are choking the peer (default)
    bool _am_interested = false;   // We want data from the peer
    bool _peer_choking = true;     // Peer is choking us (default)
    bool _peer_interested = false; // Peer wants data from us
    PeerState _state;
    asio::ip::tcp::socket _socket;
    std::shared_ptr<PieceManager> _pieceManager;
    std::vector<uint8_t> _peerBitfield;
    std::set<Block> _pendingBlocks;

    asio::awaitable<uint32_t> _readMsgLen();
    void _handleBitfield(std::span<uint8_t> payload);
    asio::awaitable<void> _handleMessage(core::msg::id msg_id, std::span<uint8_t> payload);
    asio::awaitable<void> _requestBlock();
    asio::awaitable<void> _returnBlocks();
    inline void _setState(PeerState s) {
        _state = s;
    }

    asio::awaitable<std::tuple<std::error_code, unsigned long>, asio::any_io_executor>
    _asyncWrite(const std::span<const uint8_t> data);
};
} // namespace bt