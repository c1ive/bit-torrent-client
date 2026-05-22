#include "app/file_handler.hpp"
#include <cassert>
#include <mutex>
#include <stdexcept>

namespace bt {
FileHandler::FileHandler(std::filesystem::path path, size_t pieceSize, size_t lastPieceSize)
    : _pieceSize(pieceSize), _lastPieceSize(lastPieceSize) {

    // 1. Try to open the file strictly in BINARY read/write mode
    _fileStream.open(path, std::ios::in | std::ios::out | std::ios::binary);

    // 2. If that fails (the file doesn't exist yet), create it properly
    if (!_fileStream.is_open()) {
        _fileStream.open(path, std::ios::out | std::ios::binary);
        _fileStream.close();
        _fileStream.open(path, std::ios::in | std::ios::out | std::ios::binary);
    }
}

FileHandler::~FileHandler() {
    if (_fileStream.is_open()) {
        _fileStream.close();
    }
}

void FileHandler::writePiece(uint32_t index, std::span<const uint8_t> data) {
    std::lock_guard<std::mutex> lock(_mtx);

    if (!_fileStream.is_open()) {
        throw std::runtime_error{"Failed to write to file!"};
    }

    std::streamoff offset =
        static_cast<std::streamoff>(index) * static_cast<std::streamoff>(_pieceSize);

    _fileStream.seekp(offset);
    _fileStream.write(reinterpret_cast<const char*>(data.data()), data.size());
    _fileStream.flush();
}
} // namespace bt