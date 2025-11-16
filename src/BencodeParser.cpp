#include <stdexcept>

#include "BencodeParser.hpp"

namespace bt::bencode {
constexpr char INT_START = 'i';
constexpr char LIST_START = 'l';
constexpr char DICT_START = 'd';
constexpr char END = 'e';
constexpr char COLON = ':';

Value parse(const std::string_view& data) {
    if (data.empty()) {
        throw std::invalid_argument("Empty data");
    }

    char firstChar = data[0];
    if (firstChar == INT_START) {
        return parseInt(data);
    } else if (std::isdigit(firstChar)) {
        return parseString(data);
    } else if (firstChar == LIST_START) {
        return parseList(data);
    } else if (firstChar == DICT_START) {
        return parseDict(data);
    } else {
        throw std::invalid_argument("Invalid bencode data");
    }
}

Value parseInt(const std::string_view& data) {
    // get the number between 'i' and 'e'
    size_t endPos = data.find(END, 1);
    if (endPos == std::string_view::npos) {
        throw std::invalid_argument("Invalid integer bencode");
    }
    std::string intStr = std::string(data.substr(1, endPos - 1));
    return std::stoll(intStr);
}

Value parseString(const std::string_view& data) {
    // get the length of the string
    size_t colonPos = data.find(COLON);
    if (colonPos == std::string_view::npos) {
        throw std::invalid_argument("Invalid string bencode");
    }
    std::string lenStr = std::string(data.substr(0, colonPos));
    size_t strLen = std::stoul(lenStr);

    size_t startPos = colonPos + 1;
    if (startPos + strLen > data.size()) {
        throw std::invalid_argument("String length exceeds data size");
    }

    return std::string(data.substr(startPos, strLen));
}

template <typename Container, typename Inserter>
Container parseIterableImpl(const std::string_view& data, char startChar, Inserter inserter) {
    if (data.empty() || data[0] != startChar) {
        throw std::invalid_argument("Invalid iterable bencode");
    }

    size_t pos = 1; // skip start char
    Container container;

    while (pos < data.size() && data[pos] != END) {
        inserter(container, pos, data);
    }

    return container;
}

Value parseDict(const std::string_view& data) {
    return parseIterableImpl<Dict>(
        data, DICT_START, [](Dict& dict, size_t& pos, const std::string_view& data) {
            // parse key (must be a string)
            Value key = parseString(data.substr(pos));
            if (!std::holds_alternative<std::string>(key)) {
                throw std::invalid_argument("Dictionary keys must be strings");
            }
            std::string keyStr = std::get<std::string>(key);

            // move past key in the input
            pos += std::to_string(keyStr.size()).size() + 1 + keyStr.size();

            // parse value
            Value value = parse(data.substr(pos));
            dict.items.emplace_back(keyStr, value);

            _updatePosition(value, pos, data);
        });
}

Value parseList(const std::string_view& data) {
    return parseIterableImpl<List>(data, LIST_START,
                                   [](List& list, size_t& pos, const std::string_view& data) {
                                       Value value = parse(data.substr(pos));
                                       list.values.push_back(value);
                                       _updatePosition(value, pos, data);
                                   });
}

void _updatePosition(const Value& value, size_t& pos, const std::string_view& data) {
    std::visit(
        [&pos, &data](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int64_t>) {
                pos += std::to_string(arg).size() + 2; // 'i' and 'e'
            } else if constexpr (std::is_same_v<T, std::string>) {
                pos += std::to_string(arg.size()).size() + 1 + arg.size();
            } else if constexpr (std::is_same_v<T, List>) {
                size_t listEnd = data.find(END, pos);
                pos = listEnd + 1;
            } else if constexpr (std::is_same_v<T, Dict>) {
                size_t dictEnd = data.find(END, pos);
                pos = dictEnd + 1;
            }
        },
        value);
}
} // namespace bt::bencode