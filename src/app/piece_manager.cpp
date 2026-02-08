#include "app/piece_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <openssl/sha.h>
#include <optional>
#include <vector>

namespace bt {
PieceManager::PieceManager(core::TorrentMetadata metadata, std::condition_variable& cv)
    : _metadata(metadata), _verificationHashes(_metadata.info.pieceHashes),
      _nextOffsets(_metadata.info.pieceHashes.size(), 0),
      _finished(_metadata.info.pieceHashes.size(), false),
      _bitfield((_metadata.info.pieceHashes.size() + 7) / 8, 0),
      _fileHandler("debian.iso", metadata.info.pieceLength, metadata.info.pieceLength),
      _completionCV(cv), _piecesFinished(0) {
    spdlog::debug("PieceManager initialized for {} pieces ({} bytes bitfield)",
                  _metadata.info.pieceHashes.size(), _bitfield.size());
}

std::optional<Block> PieceManager::requestBlock(std::vector<uint8_t>& peer_bitfield) {
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < _bitfield.size(); ++i) {
        uint8_t my_byte = _bitfield[i];
        uint8_t peer_byte = peer_bitfield[i];
        uint8_t needed = peer_byte & ~my_byte;

        if (needed > 0) {
            for (int bit = 7; bit >= 0; --bit) {
                if ((needed >> bit) & 1) {
                    uint32_t piece_index = (i * 8) + (7 - bit);
                    auto block = _getNextBlockForPiece(piece_index);
                    if (block) {
                        return block;
                    }
                }
            }
        }
    }

    // Peer has nothing we want
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
        spdlog::debug("Piece {} assembly complete. Verifying...", idx);

        if (_verifyHash(idx, pending.data)) {
            spdlog::debug("Piece {} verified successfully", idx);
            _fileHandler.writePiece(idx, data);
            _finished[idx] = true;
            _pendingPieces.erase(idx);
            _setPiece(idx);
            ++_piecesFinished;
            spdlog::info("Piece {} downloaded and verified.", idx);

            if (isComplete()) {
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
    spdlog::info("finished:{}, total:{}", _piecesFinished, _metadata.info.pieceHashes.size());
    return _piecesFinished >= _metadata.info.pieceHashes.size();
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