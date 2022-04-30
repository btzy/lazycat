#include <catch2/catch_test_macros.hpp>
#include <charconv>
#include <cstdint>
#include <lazycat/lazycat.hpp>
#include <limits>
#include <string_view>

using namespace lazycat;

namespace {
std::string_view sv_from_ptrs(char* begin, char* end) {
    return std::string_view(begin, end - begin);
}
}  // namespace

TEST_CASE("integral_writer int32_t") {
    auto test = [](std::int32_t val) {
        char expected[64], actual[64];
        char* expected_end = std::to_chars(expected, expected + 64, val).ptr;
        auto writer = integral_writer<std::int32_t>{{}, val, 0};
        REQUIRE(writer.size() == static_cast<size_t>(expected_end - expected));
        REQUIRE(sv_from_ptrs(actual, writer.write(actual)) == sv_from_ptrs(expected, expected_end));
    };
    test(std::numeric_limits<std::int32_t>::min());
    test(-1000000000);
    test(-999999999);
    test(-123456789);
    test(-101);
    test(-100);
    test(-99);
    test(-10);
    test(-9);
    test(-1);
    test(0);
    test(1);
    test(9);
    test(10);
    test(99);
    test(100);
    test(101);
    test(123456789);
    test(999999999);
    test(1000000000);
    test(std::numeric_limits<std::int32_t>::max());
}

TEST_CASE("integral_writer uint32_t") {
    auto test = [](std::uint32_t val) {
        char expected[64], actual[64];
        char* expected_end = std::to_chars(expected, expected + 64, val).ptr;
        auto writer = integral_writer<std::uint32_t>{{}, val, 0};
        REQUIRE(writer.size() == static_cast<size_t>(expected_end - expected));
        REQUIRE(sv_from_ptrs(actual, writer.write(actual)) == sv_from_ptrs(expected, expected_end));
    };
    test(0);
    test(1);
    test(9);
    test(10);
    test(99);
    test(100);
    test(101);
    test(123456789);
    test(999999999);
    test(1000000000);
    test(std::numeric_limits<std::uint32_t>::max() / 2);
    test(std::numeric_limits<std::uint32_t>::max() / 2 + 1);
    test(std::numeric_limits<std::uint32_t>::max());
}

TEST_CASE("integral_writer int8_t") {
    auto test = [](std::int8_t val) {
        char expected[64], actual[64];
        char* expected_end = std::to_chars(expected, expected + 64, val).ptr;
        auto writer = integral_writer<std::int8_t>{{}, val, 0};
        REQUIRE(writer.size() == static_cast<size_t>(expected_end - expected));
        REQUIRE(sv_from_ptrs(actual, writer.write(actual)) == sv_from_ptrs(expected, expected_end));
    };
    test(std::numeric_limits<std::int8_t>::min());
    test(-1);
    test(0);
    test(1);
    test(std::numeric_limits<std::int8_t>::max());
}

TEST_CASE("integral_writer uint8_t") {
    auto test = [](std::uint8_t val) {
        char expected[64], actual[64];
        char* expected_end = std::to_chars(expected, expected + 64, val).ptr;
        auto writer = integral_writer<std::uint8_t>{{}, val, 0};
        REQUIRE(writer.size() == static_cast<size_t>(expected_end - expected));
        REQUIRE(sv_from_ptrs(actual, writer.write(actual)) == sv_from_ptrs(expected, expected_end));
    };
    test(0);
    test(1);
    test(std::numeric_limits<std::uint8_t>::max());
}

TEST_CASE("concat int") {
    std::string s1 = "str1";
    int a = 10;
    int b = 12345;
    REQUIRE(cat(s1, a).build() == s1 + "10");
    REQUIRE(cat(s1, b).build() == s1 + "12345");
    REQUIRE(cat(s1, a, b).build() == s1 + "1012345");
    REQUIRE(cat(a).build() == "10");
    REQUIRE(cat(b, a).build() == "1234510");
}
