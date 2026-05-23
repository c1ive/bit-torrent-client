#pragma once

#include "app/file_handler.hpp"
#include "app/progress_tracker.hpp"
#include "core/torrent_metadata_loader.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <filesystem>
#include <map>
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

    inline bool isFinished() const {
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
    PieceManager(core::TorrentMetadata metadata, std::condition_variable& cv,
                 std::unique_ptr<ProgressTracker> progressTracker, std::filesystem::path outputPath,
                 std::mutex& completionMutex);
    ~PieceManager() = default;

    std::optional<Block> requestBlock(const std::vector<uint8_t>& peer_bitfield,
                                      const std::set<Block>& peer_pending);
    bool deliverBlock(uint32_t idx, uint32_t offset, std::span<const uint8_t> data);
    bool returnBlock(const Block& block);
    bool isComplete() const;

    inline int getTotalNumOfPieces() const {
        return _metadata.info.pieceHashes.size();
    };

private:
    std::condition_variable& _completionCV;
    std::mutex& _completionMutex;
    std::unique_ptr<bt::ProgressTracker> _progressTracker;

    FileHandler _fileHandler;
    core::TorrentMetadata _metadata;
    std::vector<uint8_t> _bitfield;
    std::mutex _mutex;

    // Different vectors for the piece states (index matching)
    std::map<uint32_t, PendingPiece> _pendingPieces;
    std::vector<core::Sha1Hash> _verificationHashes;
    std::vector<bool> _finished;
    int _piecesFinished;

    // Blocks
    std::set<Block> _pendingBlocks;
    std::vector<uint32_t> _nextOffsets;

    // Helpers
    std::optional<Block> _getNextBlockForPiece(uint32_t index);
    size_t _getPieceLength(uint32_t index) const;
    bool _verifyHash(uint32_t index, std::span<uint8_t> data) const;
    void _setPiece(uint32_t index);
};
} // namespace bt