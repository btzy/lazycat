#include <catch2/catch_test_macros.hpp>
#include <lazycat/lazycat.hpp>

TEST_CASE("basic concat strings") {
    std::string s1 = "str1";
    std::string s2 = "s2";
    std::string s3 = "string3";
    REQUIRE(lazycat().build().empty());
    REQUIRE(lazycat(s1).build() == s1);
    REQUIRE(lazycat(s1, s2).build() == s1 + s2);
    REQUIRE(lazycat(s1, s2, s3).build() == s1 + s2 + s3);
    REQUIRE(lazycat(s1).build() == static_cast<std::string>(lazycat(s1)));
    REQUIRE((lazycat() << s1 << s2 << s3).build() == s1 + s2 + s3);
}

TEST_CASE("concat char") {
    std::string s1 = "str1";
    char ch = 'x';
    REQUIRE(lazycat(ch).build() == "x");
    REQUIRE(lazycat(s1, ch).build() == s1 + ch);
    REQUIRE(lazycat(ch, s1).build() == ch + s1);
    REQUIRE(lazycat(ch, 'z').build() == "xz");
}
