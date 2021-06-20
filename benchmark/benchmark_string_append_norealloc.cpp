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

class Append5_NoRealloc_Fixture : public benchmark::Fixture {
   public:
    inline static std::string initial, first, second, third, fourth, fifth;
    void SetUp(const ::benchmark::State&) {
        initial = repeat_string("initial", 600);  // warm up the memory
        initial.resize(7 * 100);  // discard everything after the first 100 "initial"
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

BENCHMARK_F(Append5_NoRealloc_Fixture, BM_Append5_NoRealloc_Basic)(benchmark::State& state) {
    for (auto _ : state) {
        initial += first + second + third + fourth + fifth;
        benchmark::DoNotOptimize(initial);
    }
}

BENCHMARK_F(Append5_NoRealloc_Fixture, BM_Append5_NoRealloc_Better)(benchmark::State& state) {
    for (auto _ : state) {
        initial += first;
        initial += second;
        initial += third;
        initial += fourth;
        initial += fifth;
        benchmark::DoNotOptimize(initial);
    }
}

BENCHMARK_F(Append5_NoRealloc_Fixture, BM_Append5_NoRealloc_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        append(initial, first, second, third, fourth, fifth).build();
        benchmark::DoNotOptimize(initial);
    }
}

BENCHMARK_F(Append5_NoRealloc_Fixture, BM_Append5_NoRealloc_Abseil)(benchmark::State& state) {
    for (auto _ : state) {
        absl::StrAppend(&initial, first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(initial);
    }
}

BENCHMARK_F(Append5_NoRealloc_Fixture, BM_Append5_NoRealloc_DoNothing)(benchmark::State& state) {
    for (auto _ : state) {
        initial.resize(initial.size() + first.size() + second.size() + third.size() +
                       fourth.size() + fifth.size());
        benchmark::DoNotOptimize(initial);
    }
}

}  // namespace
