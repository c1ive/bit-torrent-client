#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <span>
#include <vector>

namespace bt {
class FileHandler {
public:
    FileHandler(std::filesystem::path path, size_t pieceSize, size_t lastPieceSize);
    ~FileHandler();

    void writePiece(uint32_t index, std::span<const uint8_t> data);
    std::vector<uint8_t> loadResumeStatus();
    void saveResumeStatus(const std::vector<uint8_t>& status);
    void deleteResumeStatusFile();

private:
    std::mutex _mtx;
    std::fstream _fileStream;
    std::filesystem::path _resumePath;

    size_t _pieceSize;
    size_t _lastPieceSize;
};
} // namespace bt
