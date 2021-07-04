#pragma once

#include <lazycat/lazycat_core.hpp>
#include <lazycat/util.hpp>
#include <limits>
#include <memory>
#include <type_traits>

// This file contains the writer for integral types

namespace lazycat {

namespace detail {
template <typename T>
constexpr T pow10(size_t exp) noexcept {
    T ret = 1;
    while (exp-- > 0) {
        ret *= 10;
    }
    return ret;
}

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
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size_unsigned(const T& val) noexcept {
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
            return calculate_integral_size_unsigned<Low, Mid::value>(val);
        } else {
            // Mid digits is insufficient
            return calculate_integral_size_unsigned<Mid::value, High>(val);
        }
    }
}

// Wrapper in case integer is negative
template <size_t Low, size_t High, typename T>
inline LAZYCAT_FORCEINLINE size_t calculate_integral_size(const T& val) noexcept {
    static_assert(Low < High, "Low must be less than High");
    if constexpr (std::is_signed_v<T>) {  // signed
        if (val < static_cast<T>(0)) {    // negative
            return calculate_integral_size_unsigned<Low, High>(static_cast<std::make_unsigned_t<T>>(
                       -static_cast<std::make_unsigned_t<T>>(val))) +
                   1;  // +1 for the negative sign
        } else {
            return calculate_integral_size_unsigned<Low, High>(
                static_cast<std::make_unsigned_t<T>>(val));
        }
    } else {  // unsigned
        return calculate_integral_size_unsigned<Low, High>(val);
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
                   detail::calculate_integral_size<0, std::numeric_limits<T>::digits10 + 1>(
                       content);
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
