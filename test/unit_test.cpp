#include <catch2/catch_test_macros.hpp>
#include <lazycat/lazycat.hpp>

using namespace lazycat;

TEST_CASE("basic concat strings") {
    std::string s1 = "str1";
    std::string s2 = "s2";
    std::string s3 = "string3";
    REQUIRE(cat().build().empty());
    REQUIRE(cat(s1).build() == s1);
    REQUIRE(cat(s1, s2).build() == s1 + s2);
    REQUIRE(cat(s1, s2, s3).build() == s1 + s2 + s3);
    REQUIRE(cat(s1).build() == static_cast<std::string>(cat(s1)));
    REQUIRE((cat() << s1 << s2 << s3).build() == s1 + s2 + s3);
}

TEST_CASE("concat char") {
    std::string s1 = "str1";
    char ch = 'x';
    REQUIRE(cat(ch).build() == "x");
    REQUIRE(cat(s1, ch).build() == s1 + ch);
    REQUIRE(cat(ch, s1).build() == ch + s1);
    REQUIRE(cat(ch, 'z').build() == "xz");
}
