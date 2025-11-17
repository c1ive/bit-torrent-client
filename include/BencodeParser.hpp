#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace bt::bencode {
struct List;
struct Dict;

using Value = std::variant<int64_t, std::string, List, Dict>;

struct List {
    const static char id = 'l';
    std::vector<Value> values;
};

struct Dict {
    const static char id = 'd';
    std::vector<std::pair<std::string, Value>> values;
};

template <typename Container, typename CreateItem>
Value parseContainer(const std::string& data, size_t& pos, CreateItem createItem);

Value parse(const std::string& data);
Value parse(const std::string& data, size_t& pos);

Value parseInt(const std::string& data, size_t& pos);
Value parseString(const std::string& data, size_t& pos);
Value parseList(const std::string& data, size_t& pos);
Value parseDict(const std::string& data, size_t& pos);

// Helper function to update position after parsing a value
bool _isValidBencodeInt(const std::string& s);
void _expectChar(const std::string& data, size_t& pos, char expected);
} // namespace bt::bencode