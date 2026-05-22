#include "app/file_handler.hpp"
#include <cassert>
#include <mutex>
#include <stdexcept>

namespace bt {
FileHandler::FileHandler(std::filesystem::path path, size_t pieceSize, size_t lastPieceSize)
    : _fileStream(std::fstream{path}), _resumePath(path.string() + ".resume"),
      _pieceSize(pieceSize), _lastPieceSize(lastPieceSize) {
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

    _fileStream.seekp(static_cast<std::streamoff>(index) * static_cast<std::streamoff>(_pieceSize));
    _fileStream.write(reinterpret_cast<const char*>(data.data()), data.size());
    _fileStream.flush();
}

std::vector<uint8_t> FileHandler::loadResumeStatus() {
    std::lock_guard<std::mutex> lock(_mtx);
    if (!std::filesystem::exists(_resumePath)) {
        return {};
    }

    std::ifstream resumeFile(_resumePath, std::ios::binary);
    if (!resumeFile.is_open()) {
        return {};
    }

    resumeFile.seekg(0, std::ios::end);
    std::streamsize size = resumeFile.tellg();
    resumeFile.seekg(0, std::ios::beg);

    if (size <= 0) {
        return {};
    }

    std::vector<uint8_t> status(static_cast<size_t>(size));
    resumeFile.read(reinterpret_cast<char*>(status.data()), size);
    return status;
}

void FileHandler::saveResumeStatus(const std::vector<uint8_t>& status) {
    std::lock_guard<std::mutex> lock(_mtx);
    std::ofstream resumeFile(_resumePath, std::ios::binary | std::ios::trunc);
    if (!resumeFile.is_open()) {
        throw std::runtime_error{"Failed to write resume status file!"};
    }
    resumeFile.write(reinterpret_cast<const char*>(status.data()), status.size());
    resumeFile.flush();
}

void FileHandler::deleteResumeStatusFile() {
    std::lock_guard<std::mutex> lock(_mtx);
    if (std::filesystem::exists(_resumePath)) {
        std::filesystem::remove(_resumePath);
    }
}
} // namespace bt
