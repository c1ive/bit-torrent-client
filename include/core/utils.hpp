#pragma once
#include <cstring>
#include <filesystem>
#include <netinet/in.h>
#include <span>
#include <string>
#include <vector>

namespace bt::utils {
class ByteWriter {
public:
    void write_u32(uint32_t val);
    void write_u8(uint8_t val);
    const std::vector<uint8_t>& data() const;

private:
    std::vector<uint8_t> _data;
};

class ByteReader {
public:
    explicit ByteReader(std::span<const uint8_t> buffer);

    uint32_t readU32();
    uint8_t readU8();
    std::span<const uint8_t> readRemaining();

    static std::string sha256sum(const std::filesystem::path& path);

private:
    std::span<const uint8_t> _buffer;
    size_t _cursor = 0;
};
} // namespace bt::utils
