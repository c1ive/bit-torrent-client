#include "core/utils.hpp"

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

uint8_t ByteReader::readU8() {
    if (_cursor + 1 > _buffer.size())
        throw std::out_of_range("Buffer underflow");
    return _buffer[_cursor++];
}

// Get the remaining bytes (for the actual file data)
std::span<const uint8_t> ByteReader::readRemaining() {
    return _buffer.subspan(_cursor);
}
} // namespace bt::utils