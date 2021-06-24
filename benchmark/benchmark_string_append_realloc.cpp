#include <string>
#include <string_view>

#include <absl/strings/str_cat.h>
#include <benchmark/benchmark.h>
#include <lazycat/lazycat.hpp>

using namespace lazycat;

namespace {

std::string repeat_string(std::string_view base, size_t times) {
    std::string ret;
    for (size_t i = 0; i != times; ++i) ret += base;
    return ret;
}

class Append5_Realloc_Fixture : public benchmark::Fixture {
   public:
    inline static std::string initial, first, second, third, fourth, fifth;
    void SetUp(const ::benchmark::State&) {
        initial = repeat_string("initial", 100);
        first = repeat_string("first", 100);
        second = repeat_string("second", 100);
        third = repeat_string("third", 100);
        fourth = repeat_string("fourth", 100);
        fifth = repeat_string("fifth", 100);
    }

    void TearDown(const ::benchmark::State&) {
        initial.clear();
        first.clear();
        second.clear();
        third.clear();
        fourth.clear();
        fifth.clear();
    }
};

BENCHMARK_F(Append5_Realloc_Fixture, BM_Append5_Realloc_Basic)(benchmark::State& state) {
    for (auto _ : state) {
        std::string clone = initial;
        clone.shrink_to_fit();
        clone += first + second + third + fourth + fifth;
        benchmark::DoNotOptimize(clone);
    }
}

BENCHMARK_F(Append5_Realloc_Fixture, BM_Append5_Realloc_Better)(benchmark::State& state) {
    for (auto _ : state) {
        std::string clone = initial;
        clone.shrink_to_fit();
        clone += first;
        clone += second;
        clone += third;
        clone += fourth;
        clone += fifth;
        benchmark::DoNotOptimize(clone);
    }
}

BENCHMARK_F(Append5_Realloc_Fixture, BM_Append5_Realloc_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        std::string clone = initial;
        clone.shrink_to_fit();
        append(clone, first, second, third, fourth, fifth).build();
        benchmark::DoNotOptimize(clone);
    }
}

BENCHMARK_F(Append5_Realloc_Fixture, BM_Append5_Realloc_Abseil)(benchmark::State& state) {
    for (auto _ : state) {
        std::string clone = initial;
        clone.shrink_to_fit();
        absl::StrAppend(&clone, first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(clone);
    }
}

BENCHMARK_F(Append5_Realloc_Fixture, BM_Append5_Realloc_DoNothing)(benchmark::State& state) {
    for (auto _ : state) {
        std::string clone = initial;
        clone.shrink_to_fit();
        benchmark::DoNotOptimize(clone);
    }
}

}  // namespace
