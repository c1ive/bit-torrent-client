#include <stdexcept>
#include <string>

#include "core/bencode_parser.hpp"

namespace bt::core::bencode {
Value parse(std::string_view data) {
    size_t pos = 0;
    return detail::parse(data, pos, 0);
}

std::vector<uint8_t> encode(const Value& value) {
    return std::visit(
        [](const auto& v) -> std::vector<uint8_t> {
            using T = std::decay_t<decltype(v)>;

            // integer
            if constexpr (std::is_same_v<T, int64_t>) {
                std::string tmp = "i" + std::to_string(v) + "e";
                return {tmp.begin(), tmp.end()};
            }

            // string
            else if constexpr (std::is_same_v<T, std::string>) {
                std::string tmp = std::to_string(v.size()) + ":";
                std::vector<uint8_t> res;
                res.reserve(tmp.size() + v.size());
                res.insert(res.end(), tmp.begin(), tmp.end());
                res.insert(res.end(), v.begin(), v.end());
                return res;
            }

            // list
            else if constexpr (std::is_same_v<T, List>) {
                std::vector<uint8_t> res;
                res.push_back('l');
                for (const auto& item : v.values) {
                    auto part = encode(item);
                    res.insert(res.end(), part.begin(), part.end());
                }
                res.push_back('e');
                return res;
            }

            // dict (sorted)
            else if constexpr (std::is_same_v<T, Dict>) {
                std::vector<uint8_t> res;
                res.push_back('d');

                for (const auto& [key, val] : v.values) {
                    // encode key
                    {
                        auto keyEnc = encode(key);
                        res.insert(res.end(), keyEnc.begin(), keyEnc.end());
                    }
                    // encode value
                    {
                        auto valEnc = encode(val);
                        res.insert(res.end(), valEnc.begin(), valEnc.end());
                    }
                }

                res.push_back('e');
                return res;
            }
        },
        value);
}

namespace detail {

Value parse(std::string_view data, size_t& pos, size_t depth) {
    if (data.empty()) {
        throw std::invalid_argument("Empty data");
    }

    if (depth > MAX_RECURSION_DEPTH) {
        throw std::runtime_error("Bencode recursion depth limit exceeded");
    }

    char firstChar = data[pos];
    if (firstChar == INT_START) {
        return parseInt(data, pos);
    } else if (std::isdigit(firstChar)) {
        return parseString(data, pos);
    } else if (firstChar == LIST_START) {
        return parseList(data, pos, depth);
    } else if (firstChar == DICT_START) {
        return parseDict(data, pos, depth);
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

// Parse a bencoded list
Value parseList(std::string_view data, size_t& pos, size_t depth) {
    _expectChar(data, pos, List::id);
    List list;

    while (pos < data.size() && data[pos] != END) {
        list.values.push_back(parse(data, pos, depth + 1));
    }

    _expectChar(data, pos, END);
    return list;
}

// Parse a bencoded dictionary
Value parseDict(std::string_view data, size_t& pos, size_t depth) {
    _expectChar(data, pos, Dict::id);
    Dict dict;

    while (pos < data.size() && data[pos] != END) {
        Value keyVal = parseString(data, pos);
        if (!std::holds_alternative<std::string>(keyVal))
            throw std::invalid_argument("Dict key must be a string");

        std::string key = std::get<std::string>(keyVal);
        Value val = parse(data, pos, depth + 1);
        dict.values.emplace(key, std::move(val)); // std::map insert
    }

    _expectChar(data, pos, END);
    return dict;
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
} // namespace bt::core::bencode