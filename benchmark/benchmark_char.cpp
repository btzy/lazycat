#include <string>
#include <string_view>

#include <absl/strings/str_cat.h>
#include <benchmark/benchmark.h>
#include <lazycat/lazycat.hpp>

class Add10_Char_Fixture : public benchmark::Fixture {
   public:
    inline static char first, second, third, fourth, fifth, sixth, seventh, eighth, ninth, tenth;
    void SetUp(const ::benchmark::State&) {
        first = 'a';
        second = 'b';
        third = 'c';
        fourth = 'd';
        fifth = 'e';
        sixth = 'f';
        seventh = 'g';
        eighth = 'h';
        ninth = 'i';
        tenth = 'j';
    }

    void TearDown(const ::benchmark::State&) {}
};

BENCHMARK_F(Add10_Char_Fixture, BM_Add10_Char_Basic)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = std::string{} + first + second + third + fourth + fifth + sixth +
                            seventh + eighth + ninth + tenth;
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(Add10_Char_Fixture, BM_Add10_Char_Better)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total;
        total += first;
        total += second;
        total += third;
        total += fourth;
        total += fifth;
        total += sixth;
        total += seventh;
        total += eighth;
        total += ninth;
        total += tenth;
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(Add10_Char_Fixture, BM_Add10_Char_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total =
            lazycat(first, second, third, fourth, fifth, sixth, seventh, eighth, ninth, tenth);
        benchmark::DoNotOptimize(total);
    }
}
