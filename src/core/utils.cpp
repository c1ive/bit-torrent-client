#include "core/utils.hpp"

#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

namespace bt::utils {

void ByteWriter::write_u32(uint32_t val) {
    uint32_t net = htonl(val);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&net);
    _data.insert(_data.end(), p, p + 4);
}

void ByteWriter::write_u8(uint8_t val) {
    _data.push_back(val);
}

const std::vector<uint8_t>& ByteWriter::data() const {
    return _data;
}

ByteReader::ByteReader(std::span<const uint8_t> buffer) : _buffer(buffer) {}

uint32_t ByteReader::readU32() {
    if (_cursor + 4 > _buffer.size())
        throw std::out_of_range("Buffer underflow");

    uint32_t val;
    // Copy bytes from current cursor position
    std::memcpy(&val, _buffer.data() + _cursor, 4);
    _cursor += 4; // Advance automatically

    return ntohl(val);
}

// Get the remaining bytes (for the actual file data)
std::span<const uint8_t> ByteReader::readRemaining() {
    return _buffer.subspan(_cursor);
}

std::string ByteReader::sha256sum(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for SHA256: " + path.string());
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    std::vector<char> buffer(16 * 1024);
    while (file.good()) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            SHA256_Update(&sha256, reinterpret_cast<const unsigned char*>(buffer.data()),
                          static_cast<size_t>(bytesRead));
        }
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &sha256);

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned char byte : digest) {
        hex << std::setw(2) << static_cast<int>(byte);
    }

    return hex.str();
}
} // namespace bt::utils