#include <string>
#include <string_view>

#include <benchmark/benchmark.h>
#include <lazycat/lazycat.hpp>
#include <absl/strings/str_cat.h>

std::string repeat_string(std::string_view base, size_t times) {
    std::string ret;
    for (size_t i = 0; i != times; ++i) ret += base;
    return ret;
}

static void BM_Add5_String_Basic(benchmark::State& state) {
    std::string first = repeat_string("first", 100);
    std::string second = repeat_string("second", 100);
    std::string third = repeat_string("third", 100);
    std::string fourth = repeat_string("fourth", 100);
    std::string fifth = repeat_string("fifth", 100);
    for (auto _ : state) {
        std::string total = first + second + third + fourth + fifth;
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_Add5_String_Basic);

static void BM_Add5_String_Better(benchmark::State& state) {
    std::string first = repeat_string("first", 100);
    std::string second = repeat_string("second", 100);
    std::string third = repeat_string("third", 100);
    std::string fourth = repeat_string("fourth", 100);
    std::string fifth = repeat_string("fifth", 100);
    for (auto _ : state) {
        std::string total = first;
        total += second;
        total += third;
        total += fourth;
        total += fifth;
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_Add5_String_Better);

static void BM_Add5_String_LazyCat(benchmark::State& state) {
    std::string first = repeat_string("first", 100);
    std::string second = repeat_string("second", 100);
    std::string third = repeat_string("third", 100);
    std::string fourth = repeat_string("fourth", 100);
    std::string fifth = repeat_string("fifth", 100);
    for (auto _ : state) {
        std::string total = lazycat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_Add5_String_LazyCat);

static void BM_Add5_String_Abseil(benchmark::State& state) {
    std::string first = repeat_string("first", 100);
    std::string second = repeat_string("second", 100);
    std::string third = repeat_string("third", 100);
    std::string fourth = repeat_string("fourth", 100);
    std::string fifth = repeat_string("fifth", 100);
    for (auto _ : state) {
        std::string total = absl::StrCat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_Add5_String_Abseil);

BENCHMARK_MAIN();
