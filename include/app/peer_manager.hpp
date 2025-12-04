#include <array>
#include <cstdint>
#include <vector>

namespace bt {
class PeerManager {
public:
    typedef std::array<uint8_t, 4> IpAddr;

    struct Peer {
        uint16_t port;
        IpAddr ip;

        // State, pieces, last seen, etc...
    };

    explicit PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer);
    ~PeerManager() = default;

    void start() const;

private:
    std::vector<Peer> _peers;
    static std::vector<Peer> _deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer);
};

} // namespace bt