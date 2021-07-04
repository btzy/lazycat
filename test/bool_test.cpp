#include <catch2/catch_test_macros.hpp>
#include <lazycat/lazycat.hpp>

using namespace lazycat;

TEST_CASE("bool_writer") {
    {
        const auto writer = bool_writer{{}, false};
        REQUIRE(writer.size() == 1);
        char ch;
        REQUIRE(writer.write(&ch) == (&ch) + 1);
        REQUIRE(ch == '0');
    }
    {
        const auto writer = bool_writer{{}, true};
        REQUIRE(writer.size() == 1);
        char ch;
        REQUIRE(writer.write(&ch) == (&ch) + 1);
        REQUIRE(ch == '1');
    }
}
