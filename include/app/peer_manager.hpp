#pragma once
#include "core/peer_communicator.hpp"
#include "core/torrent_metadata_loader.hpp"
#include <array>
#include <asio/detail/handler_work.hpp>
#include <asio/io_context.hpp>
#include <cstdint>
#include <vector>

namespace bt {
class PeerSession {
public:
    explicit PeerSession(asio::io_context& io_context);

    // Returns true on success, false on failure
    bool connect(const core::Peer& peer);

    void doHandshake(const core::Sha1Hash& infoHash, std::string_view peerId);
    // void requestPiece(int index, int offset, int length);
    // void readMessage();
    //  void writeMessage(...);

    // Helper to check connection status
    // bool isOpen() const {
    //     return _socket.is_open();
    // }

private:
    asio::ip::tcp::socket _socket;
};
class PeerManager {
public:
    PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer, core::Sha1Hash& infoHash,
                std::string_view peerId);
    ~PeerManager() = default;

    void start();

private:
    asio::io_context _ctx;
    std::vector<core::Peer> _peers;
    core::Sha1Hash& _infoHash;
    std::string_view _peerId;

    static std::vector<core::Peer>
    _deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer);
};

} // namespace bt