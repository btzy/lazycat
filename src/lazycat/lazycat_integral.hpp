#pragma once

#include <lazycat/lazycat_core.hpp>
#include <lazycat/util.hpp>
#include <limits>
#include <memory>
#include <type_traits>

// This file contains the writer for integral types

namespace lazycat {

namespace detail {
// Computes the number of base-2 digits in val.  Requires T to be unsigned.  If val is 0, it may
// return any number between 0 and 3 inclusive.
// For example:
//   bit_width(1) == 1
//   bit_width(2) == 2
//   bit_width(3) == 2
//   bit_width(4) == 3
//   bit_width(7) == 3
//   bit_width(8) == 4
template <typename T>
inline LAZYCAT_FORCEINLINE unsigned bit_width(const T& val) noexcept {
    static_assert(std::is_unsigned_v<T>);
#if !defined(_MSC_VER)
#if defined(__BMI__)
    if constexpr (std::numeric_limits<T>::digits <= 32) {
        return std::numeric_limits<T>::digits - _lzcnt_u32(val);
    } else if constexpr (std::numeric_limits<T>::digits <= 64) {
        return std::numeric_limits<T>::digits - static_cast<unsigned>(_lzcnt_u64(val));
    } else {
        constexpr unsigned num_steps = (std::numeric_limits<T>::digits - 1) / 64 + 1;
        for (unsigned i = num_steps - 1; i != std::numeric_limits<unsigned>::min(); --i) {
            const unsigned __int64 chunk = static_cast<unsigned __int64>(val >> (i * 64));
            if (chunk != 0)
                return i * 64 +
                       (std::numeric_limits<T>::digits - static_cast<unsigned>(_lzcnt_u64(chunk)));
        }
        return 0;
    }
#else
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<unsigned>::digits) {
        return std::numeric_limits<unsigned>::digits - __builtin_clz(val | 1);
    } else if constexpr (std::numeric_limits<T>::digits <=
                         std::numeric_limits<unsigned long>::digits) {
        return std::numeric_limits<unsigned long>::digits - __builtin_clzl(val | 1);
    } else if constexpr (std::numeric_limits<T>::digits <=
                         std::numeric_limits<unsigned long long>::digits) {
        return std::numeric_limits<unsigned long long>::digits - __builtin_clzll(val | 1);
    } else {
        constexpr unsigned num_steps =
            (std::numeric_limits<T>::digits - 1) / std::numeric_limits<unsigned long long>::digits +
            1;
        for (unsigned i = num_steps - 1; i != std::numeric_limits<unsigned>::min(); --i) {
            const unsigned long long chunk = static_cast<unsigned long long>(
                val >> (i * std::numeric_limits<unsigned long long>::digits));
            if (chunk != 0)
                return i * std::numeric_limits<unsigned long long>::digits +
                       (std::numeric_limits<unsigned long long>::digits - __builtin_clzll(chunk));
        }
        return 0;
    }
#endif
#else
    if constexpr (std::numeric_limits<T>::digits <= std::numeric_limits<unsigned>::digits) {
        return std::numeric_limits<unsigned>::digits - __lzcnt(val);
    } else if constexpr (std::numeric_limits<T>::digits <=
                         std::numeric_limits<unsigned __int64>::digits) {
        return std::numeric_limits<unsigned __int64>::digits -
               static_cast<unsigned>(__lzcnt64(val));
    } else {
        constexpr unsigned num_steps =
            (std::numeric_limits<T>::digits - 1) / std::numeric_limits<unsigned __int64>::digits +
            1;
        for (unsigned i = num_steps - 1; i != numeric_limits<unsigned>::min(); --i) {
            const unsigned __int64 chunk = static_cast<unsigned __int64>(
                val >> (i * std::numeric_limits<unsigned __int64>::digits));
            if (chunk != 0)
                return i * std::numeric_limits<unsigned __int64>::digits +
                       (std::numeric_limits<unsigned __int64>::digits -
                        static_cast<unsigned>(__lzcnt64(chunk)));
        }
        return 0;
    }
    return std::numeric_limits<T>::digits - __lzcnt(val);
#endif
}

// Stores the powers of 10
// powers_of_10[0] = 0; (special case to make 0 have length 1)
// powers_of_10[1] = 10;
// powers_of_10[2] = 100;
// powers_of_10[3] = 1000;
// ...
// powers_of_10[std::numeric_limits<T>::digits10] = 10....0;
// powers_of_10[std::numeric_limits<T>::digits10 + 1] = std::numeric_limits<T>::max();
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
static constexpr unsigned num_digits_base_2(T t) noexcept {
    static_assert(std::is_unsigned_v<T>);
    unsigned ct = 0;
    while (t > 0) {
        ++ct;
        t >>= 1;
    }
    return ct;
}

// Calculates and stores the conversion constants: approx_log10 = (approx_log2 * multiplier) >>
// rshift;
template <size_t MaxDigits, typename T>
struct log2_to_log10_converter_values {
   private:
    struct pair {
        unsigned multiplier, rshift;
    };
    // Generates the conversion for a given T and MaxDigits.
    constexpr static pair values = []() {
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
                if (i >= MaxDigits) return pair{multiplier, rshift};
            }
        }
    }();

   public:
    constexpr static unsigned multiplier = values.multiplier;
    constexpr static unsigned rshift = values.rshift;
};

template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned(const T& val) noexcept {
    static_assert(std::is_unsigned_v<T>);
    const unsigned approx_log2 = bit_width(val);
    const unsigned approx_log10 =
        (approx_log2 * log2_to_log10_converter_values<MaxDigits, T>::multiplier) >>
        log2_to_log10_converter_values<MaxDigits, T>::rshift;
    return approx_log10 + (val >= powers_of_10<T>[approx_log10]);
}

#if defined(_MSC_VER)
#pragma warning(push)
// prevents C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable : 4146)
#endif

// Wrapper in case integer is negative
// MaxDigits is the maximum number of digits it could have, excluding the '-' sign
template <size_t MaxDigits, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size(const T& val) noexcept {
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned<MaxDigits>(static_cast<std::make_unsigned_t<T>>(
                       -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned<MaxDigits>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned<MaxDigits>(val);
    }
}

template <typename T>
inline LAZYCAT_FORCEINLINE void write_integral_chars_unsigned(char* out_end, T val) noexcept {
    static_assert(std::is_unsigned_v<T> && std::is_integral_v<T>,
                  "T should be an unsigned integer");
    // Note: do-while loop ensures that zero is written as "0".
    do {
        *--out_end = '0' + static_cast<char>(val % static_cast<T>(10));
        val /= static_cast<T>(10);
    } while (val > static_cast<T>(0));
}

// Wrapper in case integer is negative
template <typename T>
inline LAZYCAT_FORCEINLINE char* write_integral_chars(char* out,
                                                      const T& val,
                                                      size_t cached_size) noexcept {
    // We write digits from back to front
    if constexpr (std::is_signed_v<T>) {  // signed
        std::make_unsigned_t<T> tmp;
        if (val < static_cast<T>(0)) {  // negative
            *out = '-';
            tmp = -static_cast<std::make_unsigned_t<T>>(val);
        } else {
            tmp = static_cast<std::make_unsigned_t<T>>(val);
        }
        out += cached_size;
        write_integral_chars_unsigned(out, tmp);
        return out;
    } else {  // unsigned
        out += cached_size;
        write_integral_chars_unsigned(out, val);
        return out;
    }
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}  // namespace detail

template <typename T>
struct integral_writer : public base_writer {
    T content;
    mutable size_t cached_size;  // cached value of size
    constexpr size_t size() const noexcept {
        return cached_size =
                   detail::calculate_integral_size<std::numeric_limits<T>::digits10 + 1>(content);
    }
    constexpr char* write(char* out) const noexcept {
        return detail::write_integral_chars(out, content, cached_size);
    }
};

// Allow only `[un]signed (char|short|int|long|long long|<extension integrals>)`, to avoid conflict
// with `char`.
template <typename Catter,
          typename T,
          typename = std::enable_if_t<std::is_base_of_v<base_catter, Catter> &&
                                      std::is_integral_v<T> && !std::is_same_v<T, bool>>,
          typename = std::enable_if_t<std::is_same_v<T, std::make_signed_t<T>> ||
                                      std::is_same_v<T, std::make_unsigned_t<T>>>>
constexpr auto operator<<(Catter c, T curr) noexcept {
    return c << integral_writer<T>{{}, std::move(curr), 0};
}

}  // namespace lazycat
