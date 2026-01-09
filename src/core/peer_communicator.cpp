#include "core/peer_communicator.hpp"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <spdlog/spdlog.h>
#include <utility>

using namespace asio::ip;
using namespace asio;

namespace bt::core {

ConnectionHandle::ConnectionHandle(const Peer& peer) : _socket(_ctx) {
    spdlog::debug("Connecting to peer {}:{}", peer.getIpStr(), peer.port);
    tcp::resolver resolver(_ctx);

    tcp::resolver::results_type endpoints =
        resolver.resolve(peer.getIpStr(), std::to_string(peer.port));

    _socket = tcp::socket(_ctx);

    auto ConnectedEndpoint = asio::connect(_socket, endpoints);
    spdlog::debug("Connected to peer at endpoint: {}", ConnectedEndpoint.address().to_string());
}

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

// Connection Handle Factory
std::unique_ptr<ConnectionHandle> connectWithPeer(const Peer& peer) {
    spdlog::debug("Establishing connection with peer {}:{}", peer.getIpStr(), peer.port);
    return std::make_unique<ConnectionHandle>(peer);
}

} // namespace bt::core