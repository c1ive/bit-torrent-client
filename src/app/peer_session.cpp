#include "app/peer_session.hpp"
#include "app/piece_manager.hpp"
#include "core/peer_communicator.hpp"
#include "core/utils.hpp"

#include <asio/awaitable.hpp>
#include <cstdint>
#include <netinet/in.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace bt {

PeerSession::PeerSession(asio::io_context& io_context, std::shared_ptr<PieceManager> pieceManager)
    : _socket(io_context), _pieceManager(pieceManager), _state(PeerState::CONNECTING) {}

asio::awaitable<uint32_t> PeerSession::_readMsgLen() {
    uint32_t network_len = 0;
    auto [ec, _] =
        co_await asio::async_read(_socket, asio::buffer(&network_len, sizeof(network_len)),
                                  asio::as_tuple(asio::use_awaitable));

    if (ec) {
        _setState(PeerState::ERROR);
        co_await _returnBlocks();
        throw std::runtime_error{ec.message()};
    }

    uint32_t len = ntohl(network_len);
    co_return len;
}

void PeerSession::_handleBitfield(std::span<uint8_t> payload) {
    int pieces = _pieceManager->getTotalNumOfPieces();
    size_t expected_size = (pieces + 7) / 8;

    if (payload.size() != expected_size) {
        spdlog::debug("Peer sent malformed bitfield size: {} (expected {})", payload.size(),
                      expected_size);
        _setState(PeerState::ERROR);
        return;
    }

    _peerBitfield.assign(payload.begin(), payload.end());

    spdlog::info("Successfully loaded bitfield from peer.");
}

asio::awaitable<void> PeerSession::_returnBlocks() {
    for (const auto& block : _pendingBlocks) {
        if (!_pieceManager->returnBlock(block)) {
            spdlog::debug("Failed to return block");
        }
    }
    co_return;
}

asio::awaitable<void> PeerSession::_requestBlock() {
    std::optional<Block> block = _pieceManager->requestBlock(_peerBitfield);
    if (!block) {
        co_return;
    }
    utils::ByteWriter msg;
    msg.write_u32(13);
    msg.write_u8(static_cast<uint8_t>(core::msg::id::REQUEST));
    msg.write_u32(block->pieceIndex);
    msg.write_u32(block->offset);
    msg.write_u32(block->length);

    auto [ec1, len] = co_await asio::async_write(_socket, asio::buffer(msg.data()),
                                                 asio::as_tuple(asio::use_awaitable));
    // TODO: Check error
    _pendingBlocks.push_back(block.value());
    spdlog::debug("Requesting block: pieceidx:{}, offset:{}, len{}", block->pieceIndex,
                 block->offset, block->length);
}

asio::awaitable<void> PeerSession::_handleMessage(core::msg::id msg_id,
                                                  std::span<uint8_t> payload) {
    using namespace core::msg;
    switch (msg_id) {
    case id::CHOKE:
        spdlog::debug("Peer choked us");
        _peer_choking = true;
        break;
    case id::UNCHOKE: {
        spdlog::debug("Peer unchoked us! We can request now.");
        _peer_choking = false;
        if (_state == PeerState::READY) {
            co_await _requestBlock();
        }
    }; break;
    case id::HAVE:
        // Parse payload (4 bytes index) and update bitfield
        break;
    case id::BITFIELD: {
        spdlog::debug("Received Bitfield of size {}", payload.size());
        _handleBitfield(payload);

        // TODO: Only send when really interested, for now we just wantn everything
        utils::ByteWriter msg;
        msg.write_u32(1);
        msg.write_u8(static_cast<uint8_t>(id::INTERESTED));
        auto [ec1, len] = co_await asio::async_write(_socket, asio::buffer(msg.data()),
                                                     asio::as_tuple(asio::use_awaitable));

        _setState(PeerState::READY);
        break;
    }
    case id::PIECE: {
        // TODO: Push block to piece manager
        utils::ByteReader reader{payload};
        uint32_t index = reader.readU32();
        uint32_t offset = reader.readU32();
        spdlog::debug("Block incoming: len:{}, idx:{}, offset:{}", payload.size(), index, offset);

        if (!_pieceManager->deliverBlock(index, offset, reader.readRemaining())) {
            co_return;
            // Varification failed -> drop peer
        }

        // Request a new block
        co_await _requestBlock();
    } break;
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
            co_await _returnBlocks();
            co_return;
        }

        core::msg::id msg_id = static_cast<core::msg::id>(message_buffer[0]);
        std::span<uint8_t> payload = {};
        if (len > 1) {
            payload = std::span<uint8_t>(message_buffer.data() + 1, len - 1);
        }

        co_await _handleMessage(msg_id, payload);
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