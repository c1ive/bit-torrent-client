#include "app/piece_manager.hpp"
#include "core/torrent_metadata_loader.hpp"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace bt {
PieceManager::PieceManager(core::TorrentMetadata metadata)
    : _metadata(metadata), _verificationHashes(_metadata.info.pieceHashes),
      _nextOffsets(_metadata.info.pieceHashes.size(), 0),
      _finished(_metadata.info.pieceHashes.size(), false),
      _bitfield((_metadata.info.pieceHashes.size() + 7) / 8, 0) {
    spdlog::debug("PieceManager initialized for {} pieces ({} bytes bitfield)",
                  _metadata.info.pieceHashes.size(), _bitfield.size());
}

std::optional<Block> PieceManager::requestBlock(std::vector<uint8_t>& peer_bitfield) {
    for (size_t i = 0; i < _bitfield.size(); ++i) {
        uint8_t my_byte = _bitfield[i];
        uint8_t peer_byte = peer_bitfield[i];
        uint8_t needed = peer_byte & ~my_byte;

        if (needed > 0) {
            for (int bit = 7; bit >= 0; --bit) {
                if ((needed >> bit) & 1) {
                    uint32_t piece_index = (i * 8) + (7 - bit);
                    return _getNextBlockForPiece(piece_index);
                }
            }
        }
    }

    // Peer has nothing we want
    return std::nullopt;
}

std::optional<Block> PieceManager::_getNextBlockForPiece(uint32_t index) {
    constexpr uint32_t BLOCK_LEN = 16384;
    uint32_t pieceLength = _metadata.info.pieceLength;

    // Handle the very last piece
    if (index == _metadata.info.pieceHashes.size() - 1) {
        uint32_t totalSize = _metadata.info.fileLength;
        uint32_t remainder = totalSize % pieceLength;
        if (remainder != 0)
            pieceLength = remainder;
    }

    // Start looking from where we last requested
    uint32_t currentOffset = _nextOffsets[index];

    // Loop until we find a gap or run out of piece
    while (currentOffset < pieceLength) {
        Block block{
            .pieceIndex = index,
            .offset = currentOffset,
            .length = std::min(BLOCK_LEN, pieceLength - currentOffset) // Clamp last block
        };

        if (!_inFlight.contains(block)) {
            _inFlight.insert(block);
            _nextOffsets[index] = currentOffset + block.length;

            return block;
        }

        currentOffset += block.length;
    }

    return std::nullopt;
}

bool PieceManager::_hasPiece(int index) const {
    int byteIndex = index / 8;
    int offset = index % 8;
    return (_bitfield[byteIndex] >> (7 - offset) & 1) != 0;
}

void PieceManager::_setPiece(int index) {
    int byteIndex = index / 8;
    int offset = index % 8;
    _bitfield[byteIndex] |= 1 << (7 - offset);
}

} // namespace bt