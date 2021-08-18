#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

#include <absl/strings/str_cat.h>
#include <benchmark/benchmark.h>
#include <fmt/core.h>
#include <lazycat/lazycat.hpp>

using namespace lazycat;

namespace {

class I32_Fixture : public benchmark::Fixture {
   public:
    inline static std::int32_t first, second, third, fourth, fifth;
    void SetUp(const ::benchmark::State&) {
        first = 12345678;
        second = 223;
        third = -5486575;
        fourth = 1;
        fifth = -1000000000;
    }

    void TearDown(const ::benchmark::State&) {}
};

BENCHMARK_F(I32_Fixture, Stringify_I32_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = cat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(I32_Fixture, Stringify_I32_Abseil)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total = absl::StrCat(first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(I32_Fixture, Stringify_I32_Fmt)(benchmark::State& state) {
    for (auto _ : state) {
        std::string total =
            fmt::format(FMT_STRING("{}{}{}{}{}"), first, second, third, fourth, fifth);
        benchmark::DoNotOptimize(total);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        const auto writer = integral_writer<std::int32_t>{{}, first, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_ToChars)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        std::to_chars(arr, arr + 64, first);
        benchmark::DoNotOptimize(arr);
    }
}

}  // namespace

namespace lazycat {

namespace detail {
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v2(const T& val) noexcept {
    T dig10 = 1;
    for (size_t i = 1; i != MaxDigits; ++i) {
        dig10 *= 10;
        if (val < dig10) return i;
    }
    return MaxDigits;
}

// Wrapper in case integer is negative
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_v2(const T& val) noexcept {
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned_v2<MaxDigits>(
                       static_cast<std::make_unsigned_t<T>>(
                       -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned_v2<MaxDigits>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned_v2<MaxDigits>(val);
    }
}
}  // namespace detail

template <typename T>
struct integral_writer_v2 : public base_writer {
    T content;
    mutable size_t cached_size;  // cached value of size
    constexpr size_t size() const noexcept {
        return cached_size =
                   detail::calculate_integral_size_v2<std::numeric_limits<T>::digits10 + 1>(
                       content);
    }
    constexpr char* write(char* out) const noexcept {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

}  // namespace lazycat

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_IterateSize)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        const auto writer = integral_writer_v2<std::int32_t>{{}, first, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::DoNotOptimize(arr);
    }
}
