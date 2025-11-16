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
    std::vector<Value> values;
};

struct Dict {
    std::vector<std::pair<std::string, Value>> items;
};

Value parse(const std::string_view& data);

Value parseInt(const std::string_view& data);
Value parseString(const std::string_view& data);
Value parseList(const std::string_view& data);
Value parseDict(const std::string_view& data);

// Helper function to update position after parsing a value
void _updatePosition(const Value& value, size_t& pos, const std::string_view& data);
} // namespace bt::bencode