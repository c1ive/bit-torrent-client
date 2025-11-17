#include <variant>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <limits>

#include "BencodeParser.hpp"

using namespace bt;

//////////////////
// Integer tests
//////////////////
TEST_CASE("Parse integer 1") {
    auto value = bencode::parse("i123e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == 123);
}

TEST_CASE("Parse integer 2") {
    auto value = bencode::parse("i-42e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == -42);
}

TEST_CASE("Parse integer 3") {
    auto value = bencode::parse("i0e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == 0);
}

TEST_CASE("Parse integer 4") {
    auto value = bencode::parse("i12093810981093e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == 12093810981093);
}

TEST_CASE("Parse integer int64 max") {
    auto value = bencode::parse("i9223372036854775807e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == std::numeric_limits<int64_t>::max());
}

TEST_CASE("Parse integer int64 min") {
    auto value = bencode::parse("i-9223372036854775808e");
    REQUIRE(std::holds_alternative<int64_t>(value));
    REQUIRE(std::get<int64_t>(value) == std::numeric_limits<int64_t>::min());
}

TEST_CASE("Parse integer negative zero") {
    REQUIRE_THROWS_AS(bencode::parse("i-0e"), std::invalid_argument);
}

TEST_CASE("Parse integer leading zero") {
    REQUIRE_THROWS_AS(bencode::parse("i0123e"), std::invalid_argument);
}

TEST_CASE("Parse integer missing end") {
    REQUIRE_THROWS_AS(bencode::parse("i123"), std::invalid_argument);
}

TEST_CASE("Parse integer empty") {
    REQUIRE_THROWS_AS(bencode::parse("ie"), std::invalid_argument);
}

TEST_CASE("Parse integer overflow") {
    REQUIRE_THROWS_AS(bencode::parse("i9223372036854775808e"), std::out_of_range);
}

TEST_CASE("Parse integer underflow") {
    REQUIRE_THROWS_AS(bencode::parse("i-9223372036854775809e"), std::out_of_range);
}

TEST_CASE("Parse invalid integer") {
    REQUIRE_THROWS_AS(bencode::parse("i12a3e"), std::invalid_argument);
}

TEST_CASE("Parse integer with plus sign") {
    REQUIRE_THROWS_AS(bencode::parse("i+3e"), std::invalid_argument);
}

TEST_CASE("Parse integer with whitespace") {
    REQUIRE_THROWS_AS(bencode::parse("i 3e"), std::invalid_argument);
}

TEST_CASE("Parse integer negative leading zero") {
    REQUIRE_THROWS_AS(bencode::parse("i-0123e"), std::invalid_argument);
}

TEST_CASE("Parse integer double minus") {
    REQUIRE_THROWS_AS(bencode::parse("i--1e"), std::invalid_argument);
}

TEST_CASE("Parse integer missing digits after minus") {
    REQUIRE_THROWS_AS(bencode::parse("i-e"), std::invalid_argument);
}

//////////////////
// String tests
//////////////////
TEST_CASE("Parse string 1") {
    auto value = bencode::parse("4:spam");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "spam");
}

TEST_CASE("Parse string 2") {
    auto value = bencode::parse("3:abc");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "abc");
}

TEST_CASE("Parse very long string") {
    std::string longStr(10000, 'a');
    std::string bencoded = std::to_string(longStr.size()) + ":" + longStr;
    auto value = bencode::parse(bencoded);
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == longStr);
}

TEST_CASE("Parse empty string") {
    auto value = bencode::parse("0:");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value).empty());
}

TEST_CASE("Parse string with colon content") {
    auto value = bencode::parse("7:foo:bar");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "foo:bar");
}

TEST_CASE("Parse string with zero length") {
    auto value = bencode::parse("0:");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value).empty());
}

TEST_CASE("Parse string with digits") {
    auto value = bencode::parse("3:123");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "123");
}

TEST_CASE("Parse string with special characters") {
    auto value = bencode::parse("5:!@#$%");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "!@#$%");
}

TEST_CASE("Parse string with escape sequences") {
    auto value = bencode::parse("6:\\n\\t\\r");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "\\n\\t\\r");
}

TEST_CASE("Parse string with null byte") {
    std::string strWithNull = std::string("abc", 3) + '\0' + std::string("def", 3);
    std::string bencoded = "7:" + strWithNull;
    auto value = bencode::parse(bencoded);
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == strWithNull);
}

TEST_CASE("Parse string with multi-digit length") {
    auto value = bencode::parse("11:hello world");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "hello world");
}

TEST_CASE("Parse string missing colon") {
    REQUIRE_THROWS_AS(bencode::parse("4spam"), std::invalid_argument);
}

TEST_CASE("Parse string length exceeds data") {
    REQUIRE_THROWS_AS(bencode::parse("5:spam"), std::invalid_argument);
}

TEST_CASE("Parse string non-digit length") {
    REQUIRE_THROWS_AS(bencode::parse("x:spam"), std::invalid_argument);
}

TEST_CASE("Parse string negative length") {
    REQUIRE_THROWS_AS(bencode::parse("-1:spam"), std::invalid_argument);
}

TEST_CASE("Parse string empty length") {
    REQUIRE_THROWS_AS(bencode::parse(":spam"), std::invalid_argument);
}

////////////////
// List tests
////////////////
TEST_CASE("Parse normal list") {
    auto value = bencode::parse("l4:spam3:abci42ee");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.size() == 3);
    REQUIRE(std::get<std::string>(list.values[0]) == "spam");
    REQUIRE(std::get<std::string>(list.values[1]) == "abc");
    REQUIRE(std::get<int64_t>(list.values[2]) == 42);
}

TEST_CASE("Parse empty list") {
    auto value = bencode::parse("le");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.empty());
}

TEST_CASE("Parse list of integers") {
    auto value = bencode::parse("li1ei2ei3ee");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.size() == 3);
    REQUIRE(std::get<int64_t>(list.values[0]) == 1);
    REQUIRE(std::get<int64_t>(list.values[1]) == 2);
    REQUIRE(std::get<int64_t>(list.values[2]) == 3);
}

TEST_CASE("Parse list of mixed types") {
    auto value = bencode::parse("l4:spami-2e0:ee");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.size() == 3);
    REQUIRE(std::get<std::string>(list.values[0]) == "spam");
    REQUIRE(std::get<int64_t>(list.values[1]) == -2);
    REQUIRE(std::get<std::string>(list.values[2]).empty());
}

TEST_CASE("Parse nested list") {
    auto value = bencode::parse("lli1ei2ee4:donee");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.size() == 2);
    REQUIRE(std::holds_alternative<bencode::List>(list.values[0]));
    const auto& inner = std::get<bencode::List>(list.values[0]);
    REQUIRE(inner.values.size() == 2);
    REQUIRE(std::get<int64_t>(inner.values[0]) == 1);
    REQUIRE(std::get<int64_t>(inner.values[1]) == 2);
    REQUIRE(std::get<std::string>(list.values[1]) == "done");
}

TEST_CASE("Parse list containing dict") {
    auto value = bencode::parse("ld3:key5:valueee");
    REQUIRE(std::holds_alternative<bencode::List>(value));
    const auto& list = std::get<bencode::List>(value);
    REQUIRE(list.values.size() == 1);
    const auto& dict = std::get<bencode::Dict>(list.values[0]);
    REQUIRE(dict.values.size() == 1);
    REQUIRE(dict.values[0].first == "key");
    REQUIRE(std::get<std::string>(dict.values[0].second) == "value");
}

TEST_CASE("Parse list missing terminator") {
    REQUIRE_THROWS_AS(bencode::parse("li1ei2e"), std::invalid_argument);
}

TEST_CASE("Parse list with invalid element") {
    REQUIRE_THROWS_AS(bencode::parse("l4spam3:abce"), std::invalid_argument);
}

////////////////
// Dict tests
////////////////
TEST_CASE("Parse simple dict") {
    auto value = bencode::parse("d3:bar4:spam3:fooi42ee");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.size() == 2);
    REQUIRE(dict.values[0].first == "bar");
    REQUIRE(std::get<std::string>(dict.values[0].second) == "spam");
    REQUIRE(dict.values[1].first == "foo");
    REQUIRE(std::get<int64_t>(dict.values[1].second) == 42);
}

TEST_CASE("Parse empty dict") {
    auto value = bencode::parse("de");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.empty());
}

TEST_CASE("Parse dict with nested list") {
    auto value = bencode::parse("d4:listli1ei2ee5:other4:donee");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.size() == 2);
    REQUIRE(dict.values[0].first == "list");
    const auto& list = std::get<bencode::List>(dict.values[0].second);
    REQUIRE(list.values.size() == 2);
    REQUIRE(std::get<int64_t>(list.values[0]) == 1);
    REQUIRE(std::get<int64_t>(list.values[1]) == 2);
    REQUIRE(dict.values[1].first == "other");
    REQUIRE(std::get<std::string>(dict.values[1].second) == "done");
}

TEST_CASE("Parse dict with nested dict") {
    auto value = bencode::parse("d4:infod3:bar3:fooe4:name4:teste");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.size() == 2);
    REQUIRE(dict.values[0].first == "info");
    const auto& inner = std::get<bencode::Dict>(dict.values[0].second);
    REQUIRE(inner.values.size() == 1);
    REQUIRE(inner.values[0].first == "bar");
    REQUIRE(std::get<std::string>(inner.values[0].second) == "foo");
    REQUIRE(dict.values[1].first == "name");
    REQUIRE(std::get<std::string>(dict.values[1].second) == "test");
}

TEST_CASE("Parse dict with list of dicts") {
    auto value = bencode::parse("d4:datald3:key5:valueed3:numi1eeee");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.size() == 1);
    REQUIRE(dict.values[0].first == "data");
    const auto& list = std::get<bencode::List>(dict.values[0].second);
    REQUIRE(list.values.size() == 2);
    const auto& firstEntry = std::get<bencode::Dict>(list.values[0]);
    REQUIRE(firstEntry.values.size() == 1);
    REQUIRE(firstEntry.values[0].first == "key");
    REQUIRE(std::get<std::string>(firstEntry.values[0].second) == "value");
    const auto& secondEntry = std::get<bencode::Dict>(list.values[1]);
    REQUIRE(secondEntry.values.size() == 1);
    REQUIRE(secondEntry.values[0].first == "num");
    REQUIRE(std::get<int64_t>(secondEntry.values[0].second) == 1);
}

/*
d
  4:root
  l
    d
      5:child
      l
        i1e
        i2e
      e
    e
  e
  4:meta
  2:ok
e
*/
TEST_CASE("Parse dict with nested list chain") {
    auto value = bencode::parse("d4:rootld5:childli1ei2eeee4:meta2:oke");
    REQUIRE(std::holds_alternative<bencode::Dict>(value));
    const auto& dict = std::get<bencode::Dict>(value);
    REQUIRE(dict.values.size() == 2);
    REQUIRE(dict.values[0].first == "root");
    const auto& outerList = std::get<bencode::List>(dict.values[0].second);
    REQUIRE(outerList.values.size() == 1);
    const auto& innerDict = std::get<bencode::Dict>(outerList.values[0]);
    REQUIRE(innerDict.values.size() == 1);
    REQUIRE(innerDict.values[0].first == "child");
    const auto& innerList = std::get<bencode::List>(innerDict.values[0].second);
    REQUIRE(innerList.values.size() == 2);
    REQUIRE(std::get<int64_t>(innerList.values[0]) == 1);
    REQUIRE(std::get<int64_t>(innerList.values[1]) == 2);
    REQUIRE(dict.values[1].first == "meta");
    REQUIRE(std::get<std::string>(dict.values[1].second) == "ok");
}

TEST_CASE("Parse dict missing terminator") {
    REQUIRE_THROWS_AS(bencode::parse("d3:bar3:foo"), std::invalid_argument);
}

TEST_CASE("Parse dict with non-string key") {
    REQUIRE_THROWS_AS(bencode::parse("di1e3:bare"), std::invalid_argument);
}

///// TODO: Torrent file tests would go here /////