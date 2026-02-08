#pragma once

#include "core/torrent_metadata_loader.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <openssl/sha.h>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace bt {

constexpr uint32_t BLOCK_LEN = 16384;

struct PendingPiece {
    std::vector<uint8_t> data;
    size_t blocksReceived = 0;
    size_t totalBlocksNeeded;

    inline bool isFinished() {
        return blocksReceived == totalBlocksNeeded;
    }
};

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
    bool deliverBlock(uint32_t idx, uint32_t offset, std::span<const uint8_t> data);
    bool returnBlock(const Block& block);

    inline int getTotalNumOfPieces() const {
        return _metadata.info.pieceHashes.size();
    };

private:
    core::TorrentMetadata _metadata;
    std::vector<uint8_t> _bitfield;
    std::mutex _mutex;

    // Different vectors for the piece states (index matching)
    std::map<uint32_t, PendingPiece> _pendingPieces;
    std::vector<core::Sha1Hash> _verificationHashes;
    std::vector<bool> _finished;

    // Blocks
    std::set<Block> _pendingBlocks;
    std::vector<uint32_t> _nextOffsets;

    // Helpers
    std::optional<Block> _getNextBlockForPiece(uint32_t index);
    size_t _getPieceLength(uint32_t index) const;
    bool _verifyHash(uint32_t index, std::span<uint8_t> data) const;

    bool _hasPiece(uint32_t index) const;
    void _setPiece(uint32_t index);
};
} // namespace bt