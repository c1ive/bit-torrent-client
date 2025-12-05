#include <array>
#include <cstdint>

namespace bt::core {
typedef std::array<uint8_t, 4> IpAddr;

struct Peer {
    uint16_t port;
    IpAddr ip;

    // State, pieces, last seen, etc...
};

void connectWithPeer(Peer& peer);
// void doHandshake(Peer& peer);
// void downloadPiece(Peer& peer);
}; // namespace bt::core