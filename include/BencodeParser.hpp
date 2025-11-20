#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
#include <stdexcept>

/**
 * @file BencodeParser.hpp
 * @brief Decode bencoded data into an in-memory representation.
 *
 * Supports integers, byte strings, lists, and dictionaries per the
 * BitTorrent bencode specification. Dictionary keys are lexicographically
 * sorted, and lists/dictionaries preserve the encoded element order.
 *
 * ---------------------------------------------------------------------------
 * Public types
 * ---------------------------------------------------------------------------
 *
 * Value
 *   std::variant<int64_t, std::string, List, Dict>
 *   Represents any decoded bencode value (integer, string, list, or dict).
 *
 * List
 *   - id: 'l'
 *   - values: elements in order
 *
 * Dict
 *   - id: 'd'
 *   - values: sequence of key/value pairs (key: std::string, value: Value)
 *
 * ---------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------------
 *
 * Value parse(std::string_view data);
 *   Decode a complete bencode payload and return the top-level Value.
 *   Throws on malformed input.
 *
 * ---------------------------------------------------------------------------
 * Internal helpers (detail namespace)
 * ---------------------------------------------------------------------------
 *
 * These functions operate on a string_view and a mutable position index (`pos`).
 * They are intended for parser implementation and are not part of the public API.
 *
 * parseContainer<Container>(data, pos, createItem)
 *   Generic parser for List or Dict containers.
 *
 * parse(data, pos)
 *   Dispatches to the correct parser based on the next character.
 *
 * parseInt(data, pos)
 *   Parse a bencode integer: 'i<digits>e'. Checks no leading zeros and '-' rules.
 *
 * parseString(data, pos)
 *   Parse length-prefixed string: '<len>:<bytes>'.
 *
 * parseList(data, pos)
 *   Parse a list into a List Value.
 *
 * parseDict(data, pos)
 *   Parse a dictionary into a Dict Value. Keys must be strings.
 *
 * _isValidBencodeInt(s)
 *   Returns true if s is a syntactically valid bencode integer payload.
 *
 * _expectChar(data, pos, expected)
 *   Advance pos if the next character matches expected; otherwise throws.
 */
namespace bt::core::bencode {
struct List;
struct Dict;

/// Any decoded bencode value
using Value = std::variant<int64_t, std::string, List, Dict>;

struct List {
    const static char id = 'l';
    std::vector<Value> values;
};

struct Dict {
    const static char id = 'd';
    std::vector<std::pair<std::string, Value>> values;
};

/// Parse a full bencode payload
Value parse(std::string_view data);

template<typename Variant, typename T>
struct is_in_variant;

template<typename... Ts, typename T>
struct is_in_variant<std::variant<Ts...>, T>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template<typename Variant, typename T>
concept InVariant = is_in_variant<Variant, T>::value;

template<typename T>
requires InVariant<Value, T>
T extractValueFromDict(const Dict& dict, const std::string& key) {
    auto it = std::find_if(dict.values.begin(), dict.values.end(),
                           [&key](const auto& pair) { return pair.first == key; });

    if (it != dict.values.end() && std::holds_alternative<T>(it->second)) {
        return std::get<T>(it->second);
    }

    throw std::runtime_error("Key '" + key + "' not found or wrong type");
}

namespace detail {
template <typename Container, typename CreateItem>
Value parseContainer(std::string_view data, size_t& pos, CreateItem createItem);

Value parse(std::string_view data, size_t& pos);
Value parseInt(std::string_view data, size_t& pos);
Value parseString(std::string_view data, size_t& pos);
Value parseList(std::string_view data, size_t& pos);
Value parseDict(std::string_view data, size_t& pos);
bool _isValidBencodeInt(std::string_view s);
void _expectChar(std::string_view data, size_t& pos, char expected);
} // namespace detail

} // namespace bt::bencode
