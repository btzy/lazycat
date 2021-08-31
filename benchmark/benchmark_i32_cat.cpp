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
        auto writer = integral_writer<std::int32_t>{{}, first, 0};
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
#if !defined(_MSC_VER)
#pragma GCC unroll 100
#else
#pragma loop(ivdep)
#endif
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
    constexpr size_t size() {
        return cached_size =
                   detail::calculate_integral_size_v2<std::numeric_limits<T>::digits10 + 1>(
                       content);
    }
    constexpr char* write(char* out) {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

namespace detail {
template <typename T>
static constexpr std::array<T, std::numeric_limits<T>::digits10 + 1> powers_of_10 = []() {
    std::array<T, std::numeric_limits<T>::digits10 + 1> powers;
    T power = 1;
    for (size_t i = 0; i < powers.size(); i++) {
        powers[i] = power;
        if (i + 1 < powers.size()) power *= 10;  // the condition prevents UB
    }
    powers[0] = 0;  // make it so that 0 is length 1
    return powers;
}();

template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v3(const T& val) noexcept {
#if !defined(_MSC_VER)
    int approx_log2 = (sizeof(unsigned int) * 8) - __builtin_clz(val | 1);
#else
    int approx_log2 = (sizeof(unsigned int) * 8) - __lzcnt(val);
#endif
    int approx_log10 = (approx_log2 * 19728) >> 16;
    return approx_log10 + (val >= powers_of_10<T>[approx_log10]);
}

// Wrapper in case integer is negative
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_v3(const T& val) noexcept {
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned_v3<MaxDigits>(
                       static_cast<std::make_unsigned_t<T>>(
                           -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned_v3<MaxDigits>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned_v3<MaxDigits>(val);
    }
}
}  // namespace detail

template <typename T>
struct integral_writer_v3 : public base_writer {
    T content;
    mutable size_t cached_size;  // cached value of size
    constexpr size_t size() {
        return cached_size =
                   detail::calculate_integral_size_v3<std::numeric_limits<T>::digits10 + 1>(
                       content);
    }
    constexpr char* write(char* out) {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

namespace detail {

template <typename T>
struct P10Entry {
    unsigned num_digits;
    T next_pow_of_10_minus_1;
};

template <typename T>
static constexpr std::array<P10Entry<T>, ((std::numeric_limits<T>::digits - 1) >> 3) + 1>
    powers_of_8 = []() {
        std::array<P10Entry<T>, ((std::numeric_limits<T>::digits - 1) >> 3) + 1> powers;
        constexpr T maxvalue = std::numeric_limits<T>::max();
        for (size_t i = 0; i < powers.size(); ++i) {
            T lowest = 1 << (i << 3);
            T num_digits = 1;
            T next_pow_of_10 = 10;
            while (true) {
                if (lowest < next_pow_of_10) {
                    --next_pow_of_10;
                    break;
                }
                ++num_digits;
                if (maxvalue / 10 < next_pow_of_10) {
                    next_pow_of_10 = maxvalue;
                    break;
                }
                next_pow_of_10 *= 10;
            }
            powers[i] = {num_digits, next_pow_of_10};
        }
        return powers;
    }();

template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v4(const T& val) noexcept {
#if !defined(_MSC_VER)
    size_t approx_log2 = (sizeof(unsigned long) * 8 - 1) - __builtin_clzl(val | 1);
#else
    size_t approx_log2 = (sizeof(unsigned int) * 8 - 1) - __lzcnt(val);
#endif
    const P10Entry<T>& entry = powers_of_8<T>[approx_log2 >> 3];
    return entry.num_digits + (val > entry.next_pow_of_10_minus_1);
}

// Wrapper in case integer is negative
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_v4(const T& val) noexcept {
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned_v4<MaxDigits>(
                       static_cast<std::make_unsigned_t<T>>(
                           -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned_v4<MaxDigits>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned_v4<MaxDigits>(val);
    }
}
}  // namespace detail

template <typename T>
struct integral_writer_v4 : public base_writer {
    T content;
    mutable size_t cached_size;  // cached value of size
    constexpr size_t size() {
        return cached_size =
                   detail::calculate_integral_size_v4<std::numeric_limits<T>::digits10 + 1>(
                       content);
    }
    constexpr char* write(char* out) {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

}  // namespace lazycat

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_IterateSize)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        auto writer = integral_writer_v2<std::int32_t>{{}, first, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ApproxSize)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        auto writer = integral_writer_v3<std::int32_t>{{}, first, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_Pow8)(benchmark::State& state) {
    for (auto _ : state) {
        char arr[64];
        auto writer = integral_writer_v4<std::int32_t>{{}, first, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v2<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_ApproxSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v3<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_Pow8)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v4<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_ToChars)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        std::to_chars(arr, arr + 64, x);
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v2<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_ApproxSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v3<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_Pow8)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v4<std::int32_t>{{}, x, 0};
        if (writer.size() <= 64) {
            writer.write(arr);
        }
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_ToChars)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        std::to_chars(arr, arr + 64, x);
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer<std::int32_t>{{}, x, 0};
        const auto size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v2<std::int32_t>{{}, x, 0};
        const auto size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_ApproxSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v3<std::int32_t>{{}, x, 0};
        const auto size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_Pow8)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v4<std::int32_t>{{}, x, 0};
        const auto size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_Original)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer<std::int32_t>{{}, x, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v2<std::int32_t>{{}, x, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_ApproxSize)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v3<std::int32_t>{{}, x, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_Pow8)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v4<std::int32_t>{{}, x, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

/* TEST_CASE("Size_I32_LazyCat_PRNG_Correctness") { */
/*     int x = 42; */
/*     for (int i = 0; i < 10000; i++) { */
/*         x = ((x >> 16) ^ x) * 0x45d9f3b; */
/*         auto writer = integral_writer<std::int32_t>{{}, x, 0}; */
/*         auto writer3 = integral_writer_v3<std::int32_t>{{}, x, 0}; */
/*         int size = writer.size(); */
/*         int size3 = writer3.size(); */
/*         REQUIRE(size == size3); */
/*     } */
/* } */
