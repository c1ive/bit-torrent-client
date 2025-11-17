#include <stdexcept>

#include "BencodeParser.hpp"

namespace bt::bencode {
Value parse(std::string_view data) {
    size_t pos = 0;
    return detail::parse(data, pos);
}

namespace detail {
constexpr char INT_START = 'i';
constexpr char LIST_START = 'l';
constexpr char DICT_START = 'd';
constexpr char END = 'e';
constexpr char COLON = ':';

Value parse(std::string_view data, size_t& pos) {
    if (data.empty()) {
        throw std::invalid_argument("Empty data");
    }

    char firstChar = data[pos];
    if (firstChar == INT_START) {
        return parseInt(data, pos);
    } else if (std::isdigit(firstChar)) {
        return parseString(data, pos);
    } else if (firstChar == LIST_START) {
        return parseList(data, pos);
    } else if (firstChar == DICT_START) {
        return parseDict(data, pos);
    } else {
        throw std::invalid_argument("Invalid bencode data");
    }
}

Value parseInt(std::string_view data, size_t& pos) {
    if (data[pos] != 'i') {
        throw std::invalid_argument("Expected int");
    }
    size_t start = pos;
    pos++; // skip 'i'

    size_t end = data.find('e', pos);
    if (end == std::string::npos) {
        throw std::invalid_argument("Bad int");
    }
    std::string num(data.substr(pos, end - pos));
    if (!_isValidBencodeInt(num)) {
        throw std::invalid_argument("Bad int");
    }
    pos = end + 1;
    return std::stoll(num);
}

Value parseString(std::string_view data, size_t& pos) {
    size_t start = pos;

    size_t colon = data.find(':', pos);
    if (colon == std::string::npos)
        throw std::invalid_argument("Bad string");

    std::string lenStr(data.substr(pos, colon - pos));
    size_t len = std::stoul(lenStr);

    pos = colon + 1;
    if (pos + len > data.size())
        throw std::invalid_argument("OOB");

    std::string value(data.substr(pos, len));
    pos += len;

    return value;
}

template <typename Container, typename CreateItem>
Value parseContainer(std::string_view data, size_t& pos, CreateItem createItem) {
    Container container;
    _expectChar(data, pos, Container::id);

    while (pos < data.size() && data[pos] != END) {
        auto item = createItem(data, pos);
        container.values.push_back(item);
    }

    _expectChar(data, pos, END);
    return container;
}

// Implementation for list
Value parseList(std::string_view data, size_t& pos) {
    return parseContainer<List>(
        data, pos, [](std::string_view data, size_t& pos) { return parse(data, pos); });
}

// Implementation for dict
Value parseDict(std::string_view data, size_t& pos) {
    return parseContainer<Dict>(data, pos, [](std::string_view data, size_t& pos) {
        Value keyVal = parseString(data, pos);
        if (!std::holds_alternative<std::string>(keyVal))
            throw std::invalid_argument("Dict key must be string");
        std::string key = std::get<std::string>(keyVal);
        Value val = parse(data, pos);
        return std::make_pair(key, val);
    });
}

bool _isValidBencodeInt(std::string_view s) {
    // Empty string is not a valid integer
    if (s.empty())
        return false;

    size_t i = 0; // Index of the first character to check

    // Handle optional negative sign
    if (s[0] == '-') {
        // "-" alone is invalid
        if (s.size() == 1)
            return false;
        // "-0" is invalid in bencode
        if (s[1] == '0')
            return false;
        i = 1; // Start checking digits after the minus sign
    }

    // Leading zero is not allowed for positive numbers, e.g., "012" is invalid
    if (s[i] == '0' && s.size() > i + 1)
        return false;

    // Check that all remaining characters are digits
    for (; i < s.size(); ++i)
        if (!isdigit(s[i]))
            return false;

    // Passed all checks, this is a valid bencode integer
    return true;
}

void _expectChar(std::string_view data, size_t& pos, char expected) {
    if (pos >= data.size() || data[pos] != expected) {
        throw std::invalid_argument(std::string("Expected '") + expected + "'");
    }
    pos++;
}
} // namespace detail
} // namespace bt::bencode