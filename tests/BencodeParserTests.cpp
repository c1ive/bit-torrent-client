#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <limits>

#include "BencodeParser.hpp"

using namespace bt;

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

TEST_CASE("Parse string") {
    auto value = bencode::parse("4:spam");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "spam");
}