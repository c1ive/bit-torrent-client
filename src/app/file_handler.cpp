#include "app/file_handler.hpp"
#include <cassert>
#include <mutex>
#include <stdexcept>

namespace bt {
FileHandler::FileHandler(std::filesystem::path path, size_t pieceSize, size_t lastPieceSize)
    : _fileStream(std::fstream{path}), _pieceSize(pieceSize), _lastPieceSize(lastPieceSize) {
    if (!_fileStream.is_open()) {
        _fileStream.open(std::string(path), std::ios::out | std::ios::binary);
        _fileStream.close();
        _fileStream.open(std::string(path), std::ios::in | std::ios::out | std::ios::binary);
    }
}

FileHandler::~FileHandler() {
    _fileStream.close();
}

void FileHandler::writePiece(uint32_t index, std::span<const uint8_t> data) {
    std::lock_guard<std::mutex> lock(_mtx);
    assert(data.size() <= _pieceSize);

    if (!_fileStream.is_open()) {
        throw std::runtime_error{"Failed to write to file!"};
    }

    _fileStream.seekp(index * _pieceSize);
    _fileStream.write(reinterpret_cast<const char*>(data.data()), data.size());
    _fileStream.flush();
}
} // namespace bt
