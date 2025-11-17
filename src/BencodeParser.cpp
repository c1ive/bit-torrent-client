#include <regex>
#include <stdexcept>

#include "BencodeParser.hpp"

namespace bt::bencode {
constexpr char INT_START = 'i';
constexpr char LIST_START = 'l';
constexpr char DICT_START = 'd';
constexpr char END = 'e';
constexpr char COLON = ':';

Value parse(const std::string& data) {
    size_t pos = 0;
    return parse(data, pos);
}

Value parse(const std::string& data, size_t& pos) {
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

Value parseInt(const std::string& data, size_t& pos) {
    if (data[pos] != 'i') {
        throw std::invalid_argument("Expected int");
    }
    size_t start = pos;
    pos++; // skip 'i'

    size_t end = data.find('e', pos);
    if (end == std::string::npos) {
        throw std::invalid_argument("Bad int");
    }
    std::string num = data.substr(pos, end - pos);
    if (!_isValidBencodeInt(num)) {
        throw std::invalid_argument("Bad int");
    }
    pos = end + 1;
    return std::stoll(num);
}

Value parseString(const std::string& data, size_t& pos) {
    size_t start = pos;

    size_t colon = data.find(':', pos);
    if (colon == std::string::npos)
        throw std::invalid_argument("Bad string");

    std::string lenStr = data.substr(pos, colon - pos);
    size_t len = std::stoul(lenStr);

    pos = colon + 1;
    if (pos + len > data.size())
        throw std::invalid_argument("OOB");

    std::string value = data.substr(pos, len);
    pos += len;

    return value;
}

Value parseDict(const std::string& data, size_t& pos) {
    if (data[pos] != 'd') {
        throw std::invalid_argument("Expected dict");
    }

    pos++; // skip 'd'
    Dict dict;

    while (pos < data.size() && data[pos] != END) {
        Value keyVal = parseString(data, pos);
        if (!std::holds_alternative<std::string>(keyVal)) {
            throw std::invalid_argument("Dictionary keys must be strings");
        }
        std::string key = std::get<std::string>(keyVal);

        Value value = parse(data, pos);
        dict.items.emplace_back(key, value);
    }

    if (pos >= data.size() || data[pos] != END) {
        throw std::invalid_argument("Unterminated dict");
    }
    pos++; // skip 'e'
    return dict;
}

Value parseList(const std::string& data, size_t& pos) {
    if (data[pos] != LIST_START) {
        throw std::invalid_argument("Expected list");
    }

    pos++; // skip 'l'
    List list;

    while (pos < data.size() && data[pos] != END) {
        Value res = parse(data, pos);
        list.values.push_back(res);
    }

    if (pos >= data.size() || data[pos] != END) {
        throw std::invalid_argument("Unterminated list");
    }
    pos++;       // skip 'e'
    return list;
}

bool _isValidBencodeInt(const std::string& s) {
    static const std::regex re("^(0|-[1-9][0-9]*|[1-9][0-9]*)$");
    return std::regex_match(s, re);
}
} // namespace bt::bencode