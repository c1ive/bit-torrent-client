#include "core/peer_communicator.hpp"

#include <asio/ip/tcp.hpp>
#include <utility>

using namespace asio::ip;
using namespace asio;

namespace bt::core {

ConnectionHandle::ConnectionHandle(io_context& io) : _socket(io) {}

tcp::socket& ConnectionHandle::socket() {
    return _socket;
}

bool ConnectionHandle::isOpen() const {
    return _socket.is_open();
}

void ConnectionHandle::close() {
    _socket.close();
}

ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept
    : _socket(std::move(other._socket)) {}

ConnectionHandle& ConnectionHandle::operator=(ConnectionHandle&& other) noexcept {
    if (this != &other) {
        _socket = std::move(other._socket);
    }
    return *this;
}

PeerSession connectWithPeer(Peer& peer) {
    io_context io_context;
    tcp::resolver resolver(io_context);

    tcp::resolver::results_type endpoints =
        resolver.resolve(peer.getIpStr(), std::to_string(peer.port));
}

} // namespace bt::core