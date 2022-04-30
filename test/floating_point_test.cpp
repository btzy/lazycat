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

TEST_CASE("floating_point_writer double") {
    auto test = [](double val) {
        char expected[64], actual[64];
        char* expected_end = std::to_chars(expected, expected + 64, val).ptr;
        auto writer = floating_point_writer<double>{{}, val, {}};
        REQUIRE(writer.size() == static_cast<size_t>(expected_end - expected));
        REQUIRE(sv_from_ptrs(actual, writer.write(actual)) == sv_from_ptrs(expected, expected_end));
    };
    test(-std::numeric_limits<double>::infinity());
    test(std::numeric_limits<double>::min());
    test(-2e100);
    test(-2.12e100);
    test(-3243.454);
    test(-1);
    test(-1.4e-3);
    test(-1.44e-23);
    test(0);
    test(0.0001);
    test(1);
    test(3);
    test(-9);
    test(-1);
    test(0);
    test(1);
    test(3.14159265358979323);
    test(10);
    test(99.99);
    test(1e5);
    test(2.3456757e+55);
    test(std::numeric_limits<double>::max());
    test(std::numeric_limits<double>::infinity());
    test(std::numeric_limits<double>::quiet_NaN());
}
