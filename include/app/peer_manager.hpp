#include <array>
#include <cstdint>
#include <vector>
#include "core/peer_communicator.hpp"

namespace bt {
class PeerManager {
public:


    explicit PeerManager(std::vector<std::array<uint8_t, 6>> peerBuffer);
    ~PeerManager() = default;

    void start() const;

private:
    std::vector<core::Peer> _peers;
    static std::vector<core::Peer> _deserializePeerBuffer(const std::vector<std::array<uint8_t, 6>>& peerBuffer);
};

} // namespace bt