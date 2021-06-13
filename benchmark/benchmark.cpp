#include <string>
#include <string_view>

#include <absl/strings/str_cat.h>
#include <benchmark/benchmark.h>
#include <lazycat/lazycat.hpp>

std::string repeat_string(std::string_view base, size_t times) {
    std::string ret;
    for (size_t i = 0; i != times; ++i) ret += base;
    return ret;
}

class Add5_String_Fixture : public benchmark::Fixture {
   public:
    inline static std::string first, second, third, fourth, fifth;
    void SetUp(const ::benchmark::State&) {
        first = repeat_string("first", 100);
        second = repeat_string("second", 100);
        third = repeat_string("third", 100);
        fourth = repeat_string("fourth", 100);
        fifth = repeat_string("fifth", 100);
    }

    void TearDown(const ::benchmark::State&) {
        first.clear();
        second.clear();
        third.clear();
        fourth.clear();
        fifth.clear();
    }
};

BENCHMARK_F(Add5_String_Fixture, BM_Add5_String_Basic)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = first + second + third + fourth + fifth;
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(Add5_String_Fixture, BM_Add5_String_Better)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = first;
        total += second;
        total += third;
        total += fourth;
        total += fifth;
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(Add5_String_Fixture, BM_Add5_String_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = lazycat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(Add5_String_Fixture, BM_Add5_String_Abseil)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = absl::StrCat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_MAIN();
