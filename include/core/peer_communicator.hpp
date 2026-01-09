#include <array>
#include <asio/io_context.hpp>
#include <cstdint>

#include <asio.hpp>
#include <format>
#include <spdlog/spdlog.h>
#include <string>

namespace bt::core {
typedef std::array<uint8_t, 4> IpAddr;

struct Peer {
    uint16_t port;
    IpAddr ip;

    std::string getIpStr() const {
        const auto& f = [this](int x) { return ip[x]; };
        auto res = std::format("{}.{}.{}.{}", f(0), f(1), f(2), f(3));
        return res;
    }
};

class ConnectionHandle {
public:
    ConnectionHandle(const Peer& peer);

    asio::ip::tcp::socket& socket();

    bool isOpen() const;

    void close();

    // non-copyable, movable
    ConnectionHandle(const ConnectionHandle&) = delete;
    ConnectionHandle& operator=(const ConnectionHandle&) = delete;

    ConnectionHandle(ConnectionHandle&& other) noexcept;

    ConnectionHandle& operator=(ConnectionHandle&& other) noexcept;

private:
    asio::io_context _ctx;
    asio::ip::tcp::socket _socket;
};

class PeerSession {
public:
    PeerSession(ConnectionHandle& handle);

    void doHandshake(const Peer& peer);
    void requestPiece(int index, int offset, int length);
    void readMessage();
    void writeMessage(...);

private:
    ConnectionHandle& _handle;
};

std::unique_ptr<ConnectionHandle> connectWithPeer(const Peer& peer);
PeerSession startPeerSession(ConnectionHandle& handle);
}; // namespace bt::core