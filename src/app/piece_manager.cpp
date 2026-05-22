#include "app/piece_manager.hpp"
#include "app/progress_tracker.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <openssl/sha.h>
#include <optional>
#include <vector>

namespace bt {
PieceManager::PieceManager(core::TorrentMetadata metadata, std::condition_variable& cv,
                           std::unique_ptr<bt::ProgressTracker> progressTracker,
                           std::filesystem::path outputPath)
    : _metadata(metadata), _verificationHashes(_metadata.info.pieceHashes),
      _nextOffsets(_metadata.info.pieceHashes.size(), 0),
      _finished(_metadata.info.pieceHashes.size(), false),
      _pieceAvailability(_metadata.info.pieceHashes.size(), 0),
      _bitfield((_metadata.info.pieceHashes.size() + 7) / 8, 0),
      _fileHandler(std::move(outputPath), metadata.info.pieceLength, metadata.info.pieceLength),
      _completionCV(cv), _piecesFinished(0), _progressTracker(std::move(progressTracker)) {
    const auto resumeStatus = _fileHandler.loadResumeStatus();
    if (!resumeStatus.empty()) {
        std::vector<uint8_t> normalized(resumeStatus);
        normalized.resize(_bitfield.size());
        _bitfield = normalized;

        for (uint32_t pieceIndex = 0; pieceIndex < _metadata.info.pieceHashes.size();
             ++pieceIndex) {
            if (_hasPiece(pieceIndex)) {
                _finished[pieceIndex] = true;
                ++_piecesFinished;
            }
        }
        if (_progressTracker) {
            _progressTracker->setFinishedPieces(_piecesFinished);
            _progressTracker->setResumeMessage(_piecesFinished);
        }
        spdlog::info("Resuming download; {} pieces already complete.", _piecesFinished);
    }

    spdlog::debug("PieceManager initialized for {} pieces ({} bytes bitfield)",
                  _metadata.info.pieceHashes.size(), _bitfield.size());
}

std::optional<Block> PieceManager::requestBlock(std::vector<uint8_t>& peer_bitfield) {
    std::lock_guard<std::mutex> lock(_mutex);
    size_t totalPieces = _metadata.info.pieceHashes.size();
    std::vector<uint32_t> candidates;
    candidates.reserve(totalPieces);

    for (uint32_t pieceIndex = 0; pieceIndex < totalPieces; ++pieceIndex) {
        if (_finished[pieceIndex]) {
            continue;
        }

        int byteIndex = pieceIndex / 8;
        int bitIndex = pieceIndex % 8;
        if (byteIndex >= static_cast<int>(peer_bitfield.size())) {
            continue;
        }

        bool peerHasPiece = ((peer_bitfield[byteIndex] >> (7 - bitIndex)) & 1) != 0;
        if (!peerHasPiece) {
            continue;
        }

        candidates.push_back(pieceIndex);
    }

    if (candidates.empty()) {
        return std::nullopt;
    }

    std::sort(candidates.begin(), candidates.end(), [this](uint32_t a, uint32_t b) {
        int availabilityA = _pieceAvailability[a];
        int availabilityB = _pieceAvailability[b];
        if (availabilityA != availabilityB) {
            return availabilityA < availabilityB;
        }
        bool aInProgress = _pendingPieces.contains(a);
        bool bInProgress = _pendingPieces.contains(b);
        if (aInProgress != bInProgress) {
            return !aInProgress;
        }
        return a < b;
    });

    for (uint32_t pieceIndex : candidates) {
        auto block = _getNextBlockForPiece(pieceIndex);
        if (block) {
            return block;
        }
    }

    return std::nullopt;
}

bool PieceManager::deliverBlock(uint32_t idx, uint32_t offset, std::span<const uint8_t> data) {
    std::lock_guard<std::mutex> lock(_mutex);

    if (_finished[idx]) {
        return true;
    }

    if (!_pendingPieces.contains(idx)) {
        size_t len = _getPieceLength(idx);
        size_t totalBlocks = (len + BLOCK_LEN - 1) / BLOCK_LEN;

        _pendingPieces[idx] = PendingPiece{.data = std::vector<uint8_t>(len),
                                           .blocksReceived = 0,
                                           .totalBlocksNeeded = totalBlocks};
    }

    auto& pending = _pendingPieces[idx];

    if (offset + data.size() > pending.data.size()) {
        spdlog::debug("Received block out of bounds for piece {}", idx);
        return false;
    }

    std::copy_n(data.data(), data.size(), pending.data.data() + offset);
    pending.blocksReceived++;

    Block finishedBlock{
        .pieceIndex = idx,
        .offset = offset,
        .length = std::min(BLOCK_LEN, static_cast<uint32_t>(pending.data.size()) - offset)};
    _pendingBlocks.erase(finishedBlock);

    if (pending.isFinished()) {
        if (_verifyHash(idx, pending.data)) {
            _fileHandler.writePiece(idx, pending.data);
            _finished[idx] = true;
            _pendingPieces.erase(idx);
            _setPiece(idx);
            ++_piecesFinished;
            _fileHandler.saveResumeStatus(_bitfield);

            if (_progressTracker) {
                _progressTracker->notifyProgress();
            }

            spdlog::info("Piece {} downloaded and verified.", idx);

            if (isComplete()) {
                _fileHandler.deleteResumeStatusFile();
                // Wake up torren orchestrator
                _completionCV.notify_one();
            }
            return true;
        } else {
            spdlog::warn("Piece {} Hash Mismatch! Discarding.", idx);
            _pendingPieces.erase(idx); // Throw it away
            _nextOffsets[idx] = 0;
            return false;
        }
    }

    return true;
}

bool PieceManager::returnBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pendingBlocks.contains(block)) {
        if (block.offset < _nextOffsets[block.pieceIndex]) {
            _nextOffsets[block.pieceIndex] = block.offset;
        }
        _pendingBlocks.erase(block);
        return true;
    } else {
        return false;
    }
}

bool PieceManager::_verifyHash(uint32_t index, std::span<uint8_t> data) const {
    auto expectedHash = _verificationHashes[index];
    core::Sha1Hash calculatedHash;
    SHA1(data.data(), data.size(), calculatedHash.data());
    return expectedHash == calculatedHash;
};

bool PieceManager::isComplete() {
    return _piecesFinished >= _metadata.info.pieceHashes.size();
}

void PieceManager::registerPeerBitfield(size_t peerKey, const std::vector<uint8_t>& peerBitfield) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (auto it = _peerBitfields.find(peerKey); it != _peerBitfields.end()) {
        _updateAvailability(it->second, -1);
        it->second = peerBitfield;
        _updateAvailability(peerBitfield, 1);
        return;
    }

    _peerBitfields.emplace(peerKey, peerBitfield);
    _updateAvailability(peerBitfield, 1);
}

void PieceManager::updatePeerBitfield(size_t peerKey, const std::vector<uint8_t>& peerBitfield) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (auto it = _peerBitfields.find(peerKey); it != _peerBitfields.end()) {
        _updateAvailability(it->second, -1);
        it->second = peerBitfield;
        _updateAvailability(peerBitfield, 1);
        return;
    }

    _peerBitfields.emplace(peerKey, peerBitfield);
    _updateAvailability(peerBitfield, 1);
}

void PieceManager::unregisterPeerBitfield(size_t peerKey) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (auto it = _peerBitfields.find(peerKey); it != _peerBitfields.end()) {
        _updateAvailability(it->second, -1);
        _peerBitfields.erase(it);
    }
}

void PieceManager::_updateAvailability(const std::vector<uint8_t>& bitfield, int delta) {
    size_t totalPieces = _metadata.info.pieceHashes.size();
    for (uint32_t pieceIndex = 0; pieceIndex < totalPieces; ++pieceIndex) {
        int byteIndex = pieceIndex / 8;
        int bitIndex = pieceIndex % 8;
        if (byteIndex >= static_cast<int>(bitfield.size())) {
            break;
        }

        bool peerHasPiece = ((bitfield[byteIndex] >> (7 - bitIndex)) & 1) != 0;
        if (peerHasPiece) {
            _pieceAvailability[pieceIndex] += delta;
        }
    }
}

std::optional<Block> PieceManager::_getNextBlockForPiece(uint32_t index) {
    uint32_t pieceLength = _getPieceLength(index);
    uint32_t currentOffset = _nextOffsets[index];

    // Loop until we find a gap or run out of piece
    while (currentOffset < pieceLength) {
        Block block{
            .pieceIndex = index,
            .offset = currentOffset,
            .length = std::min(BLOCK_LEN, pieceLength - currentOffset) // Clamp last block
        };

        if (!_pendingBlocks.contains(block)) {
            _pendingBlocks.insert(block);
            _nextOffsets[index] = currentOffset + block.length;

            return block;
        }

        currentOffset += block.length;
    }

    return std::nullopt;
}

size_t PieceManager::_getPieceLength(uint32_t index) const {
    uint32_t pieceLength = _metadata.info.pieceLength;

    // Handle the very last piece
    if (index == _metadata.info.pieceHashes.size() - 1) {
        uint32_t totalSize = _metadata.info.fileLength;
        uint32_t remainder = totalSize % pieceLength;
        if (remainder != 0)
            pieceLength = remainder;
    }

    return pieceLength;
}

bool PieceManager::_hasPiece(uint32_t index) const {
    int byteIndex = index / 8;
    int offset = index % 8;
    return (_bitfield[byteIndex] >> (7 - offset) & 1) != 0;
}

void PieceManager::_setPiece(uint32_t index) {
    int byteIndex = index / 8;
    int offset = index % 8;
    _bitfield[byteIndex] |= 1 << (7 - offset);
}

} // namespace bt