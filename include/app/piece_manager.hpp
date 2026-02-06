#pragma once

#include "core/torrent_metadata_loader.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <vector>

namespace bt {
struct Block {
    uint32_t pieceIndex;
    uint32_t offset;
    uint32_t length;
    bool operator<(const Block& other) const {
        return std::tie(pieceIndex, offset) < std::tie(other.pieceIndex, other.offset);
    }
};

class PieceManager {
public:
    PieceManager(core::TorrentMetadata metadata);
    ~PieceManager() = default;

    std::optional<Block> requestBlock(std::vector<uint8_t>& peer_bitfield);
    inline int getTotalNumOfPieces() const {
        return _metadata.info.pieceHashes.size();
    };

private:
    core::TorrentMetadata _metadata;
    std::vector<uint8_t> _bitfield;

    // Different vectors for the piece states (index matching)
    std::set<Block> _inFlight;
    std::vector<bool> _finished;
    std::vector<uint32_t> _nextOffsets;
    std::vector<core::Sha1Hash> _verificationHashes;

    std::optional<Block> _getNextBlockForPiece(uint32_t index);

    bool _hasPiece(int index) const;
    void _setPiece(int intex);
};
} // namespace bt