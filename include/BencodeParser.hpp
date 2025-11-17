#pragma once

#include <cstdint>
#include <string>
#include <string_view>
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
Value parseContainer(std::string_view data, size_t& pos, CreateItem createItem);

Value parse(std::string_view data);
Value parse(std::string_view data, size_t& pos);

Value parseInt(std::string_view data, size_t& pos);
Value parseString(std::string_view data, size_t& pos);
Value parseList(std::string_view data, size_t& pos);
Value parseDict(std::string_view data, size_t& pos);

// Helper function to update position after parsing a value
bool _isValidBencodeInt(std::string_view s);
void _expectChar(std::string_view data, size_t& pos, char expected);
} // namespace bt::bencode