#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

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

TEST_CASE("Parse integer overflow") {
    REQUIRE_THROWS_AS(bencode::parse("i9223372036854775808e"), std::out_of_range);
}

TEST_CASE("Parse invalid integer") {
    REQUIRE_THROWS_AS(bencode::parse("i12a3e"), std::invalid_argument);
}

TEST_CASE("Parse string") {
    auto value = bencode::parse("4:spam");
    REQUIRE(std::holds_alternative<std::string>(value));
    REQUIRE(std::get<std::string>(value) == "spam");
}