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
#if defined(_MSC_VER)
#pragma warning(push)
// prevents C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable : 4146)
#endif

// Binary search to find the number of digits in the given integral.
// Low: Something that is guaranteed to be too small
// High: Something that is guaranteed to be large enough
// Expects: Low < High
// Returns: the smallest value that is large enough (will be between Low+1 and High inclusive)
// Note: 0 is always too small, because the number 0 has size 1.
template <size_t Low, size_t High, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v1(const T& val) noexcept {
    static_assert(Low < High, "Low must be less than High");
    static_assert(std::is_unsigned_v<T> && std::is_integral_v<T>,
                  "T should be an unsigned integer");
    if constexpr (Low + 1 == High) {
        return High;
    } else {
        using Mid = std::integral_constant<size_t, (Low + High) / 2>;
        using MidValue = std::integral_constant<T, pow10<T>(Mid::value)>;  // 10^Mid (Mid+1 digits)
        if (val < MidValue::value) {
            // Mid digits is sufficient
            return calculate_integral_size_unsigned_v1<Low, Mid::value>(val);
        } else {
            // Mid digits is insufficient
            return calculate_integral_size_unsigned_v1<Mid::value, High>(val);
        }
    }
}

// Wrapper in case integer is negative
template <size_t Low, size_t High, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_v1(const T& val) noexcept {
    static_assert(Low < High, "Low must be less than High");
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned_v1<Low, High>(
                       static_cast<std::make_unsigned_t<T>>(
                           -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned_v1<Low, High>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned<Low, High>(val);
    }
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}  // namespace detail

template <typename T>
struct integral_writer_v1 : public base_writer {
    T content;
    mutable size_t cached_size;  // cached value of size
    constexpr size_t size() const noexcept {
        return cached_size =
                   detail::calculate_integral_size_v1<0, std::numeric_limits<T>::digits10 + 1>(
                       content);
    }
    constexpr char* write(char* out) const noexcept {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

namespace detail {
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v2(const T& val) noexcept {
    T dig10 = 1;
#if defined(_MSC_VER)
#pragma loop(ivdep)
#elif defined(__clang__)
#pragma unroll
#else
#pragma GCC unroll 100
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
// Stores the powers of 10
// powers_of_10[0] = 0; (special case to make 0 have length 1)
// powers_of_10[1] = 10;
// powers_of_10[2] = 100;
// powers_of_10[3] = 1000;
// ...
template <typename T>
static constexpr std::array<T, std::numeric_limits<T>::digits10 + 1> powers_of_10 = []() {
    std::array<T, std::numeric_limits<T>::digits10 + 1> powers{};
    T power = 1;
    for (size_t i = 0; i < powers.size(); i++) {
        powers[i] = power;
        if (i + 1 < powers.size()) power *= 10;  // the condition prevents UB
    }
    powers[0] = 0;  // make it so that 0 is length 1
    return powers;
}();

template <typename T>
static constexpr unsigned num_digits_base_2(T t) {
    static_assert(std::is_unsigned_v<T>);
    unsigned ct = 0;
    while (t > 0) {
        ++ct;
        t >>= 1;
    }
    return ct;
}

// Stores the conversion: approx_log10 = (approx_log2 * multiplier) >> rshift;
struct log2_to_log10_converter_values_t {
    unsigned multiplier;
    unsigned rshift;
};

// Generates the conversion for a given T and MaxDigits.
template <size_t MaxDigits, typename T>
static constexpr log2_to_log10_converter_values_t log2_to_log10_converter_values = []() {
    for (unsigned rshift = 0; true; ++rshift) {
        const unsigned lower_bound = ((2u << rshift) - 1) / 7 + 1;
        const unsigned upper_bound = ((3u << rshift) - 1) / 7;
        for (unsigned multiplier = lower_bound; multiplier <= upper_bound; ++multiplier) {
            unsigned i;
            for (i = 1; i < MaxDigits; ++i) {
                const unsigned approx_log10 =
                    (num_digits_base_2(powers_of_10<T>[i]) * multiplier) >> rshift;
                if (approx_log10 != i) break;
            }
            // MSVC can't handle 'continue' in constexpr context
            if (i >= MaxDigits) return log2_to_log10_converter_values_t{multiplier, rshift};
        }
    }
}();

template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned_v3(const T& val) noexcept {
    static_assert(std::is_unsigned_v<T>);
#if !defined(_MSC_VER)
#if defined(__BMI__)
    unsigned int approx_log2 = std::numeric_limits<T>::digits - _lzcnt_u32(val);
#else
    unsigned int approx_log2 = std::numeric_limits<T>::digits - __builtin_clz(val | 1);
#endif
#else
    unsigned int approx_log2 = std::numeric_limits<T>::digits - __lzcnt(val);
#endif
    // static_assert(log2_to_log10_converter_values<MaxDigits, T>.multiplier == 5 &&
    //               log2_to_log10_converter_values<MaxDigits, T>.rshift == 4);
    unsigned int approx_log10 =
        (approx_log2 * log2_to_log10_converter_values<MaxDigits, T>.multiplier) >>
        log2_to_log10_converter_values<MaxDigits, T>.rshift;
    return approx_log10 + (val >= powers_of_10<T>[approx_log10]);
}

// Wrapper in case integer is negative
// MaxDigits is the maximum number of digits it could have, excluding the '-' sign
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
        std::array<P10Entry<T>, ((std::numeric_limits<T>::digits - 1) >> 3) + 1> powers{};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_PRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v1<std::int32_t>{{}, x, 0};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
    }
}

BENCHMARK_F(I32_Fixture, Write_I32_LazyCat_ExpPRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        char arr[64];
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v1<std::int32_t>{{}, y, 0};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v2<std::int32_t>{{}, y, 0};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v3<std::int32_t>{{}, y, 0};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v4<std::int32_t>{{}, y, 0};
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        std::to_chars(arr, arr + 64, y);
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(arr);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_Original)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        auto writer = integral_writer_v1<std::int32_t>{{}, x, 0};
        const auto size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_PRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
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
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v1<std::int32_t>{{}, y, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_IterateSize)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v2<std::int32_t>{{}, y, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_ApproxSize)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v3<std::int32_t>{{}, y, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

BENCHMARK_F(I32_Fixture, Size_I32_LazyCat_ExpPRNG_Pow8)(benchmark::State& state) {
    int x = 42;
    size_t size;
    for (auto _ : state) {
        x = ((x >> 16) ^ x) * 0x45d9f3c;
        int y = detail::powers_of_10<int>[static_cast<unsigned int>(x) % 9] & x;
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
        auto writer = integral_writer_v4<std::int32_t>{{}, y, 0};
        size = writer.size();
        benchmark::ClobberMemory();
        benchmark::DoNotOptimize(size);
    }
}

/* TEST_CASE("Size_I32_LazyCat_PRNG_Correctness") { */
/*     int x = 42; */
/*     for (int i = 0; i < 10000; i++) { */
/*         x = ((x >> 16) ^ x) * 0x45d9f3c; */
/*         auto writer = integral_writer<std::int32_t>{{}, x, 0}; */
/*         auto writer3 = integral_writer_v3<std::int32_t>{{}, x, 0}; */
/*         int size = writer.size(); */
/*         int size3 = writer3.size(); */
/*         REQUIRE(size == size3); */
/*     } */
/* } */
