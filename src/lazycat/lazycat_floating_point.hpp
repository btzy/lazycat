#pragma once

#if __has_include(<charconv>)
#include <charconv>
#endif
#if !(defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L)
#include <cstdio>
#endif
#include <lazycat/lazycat_core.hpp>

// This file contains the writer for floating point types.  It simply calls std::to_chars (which
// hopefully uses something fast like Ryu).  If std::to_chars is not available, then it falls back
// on std::sprintf("%g") (which is not exactly equivalent to std::to_chars).

namespace lazycat {

namespace detail {

// This function is only used at compile time.
constexpr int log10_ceil(int num) noexcept {
    return num < 10 ? 1 : 1 + log10_ceil(num / 10);
}

// This function is only used at compile time.
constexpr int round_up_to_multiple(size_t num, size_t multiple) noexcept {
    return (num + (multiple - 1)) / multiple * multiple;
}

#if !(defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811)
constexpr int sprintf_floating_point(char* buf, float val) {
    return std::sprintf(buf, "%g", static_cast<double>(val));
}
constexpr int sprintf_floating_point(char* buf, double val) {
    return std::sprintf(buf, "%g", val);
}
constexpr int sprintf_floating_point(char* buf, long double val) {
    return std::sprintf(buf, "%Lg", val);
}
#endif

}  // namespace detail

template <typename T>
struct floating_point_writer : public base_writer {
    T content;
    constexpr static size_t buffer_size = round_up_to_multiple(
        static_cast<size_t>(
            4 + std::numeric_limits<T>::max_digits10 +
            std::max(2, detail::log10_ceil(std::numeric_limits<T>::max_exponent10))),
        alignof(T));
    alignas(T) mutable char cached_buffer[buffer_size];
    mutable size_t cached_size;
    constexpr size_t size() const noexcept {
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
        char* const end = std::to_chars(cached_buffer, cached_buffer + buffer_size, content).ptr;
        return cached_size = end - cached_buffer;
#else
        const int ct = sprintf_floating_point(cached_buffer, content);
        return cached_size = ct;
#endif
    }
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811
    constexpr char* write(char* out) const noexcept {
        if (std::is_constant_evaluated()) {
            return std::copy(cached_buffer, cached_buffer + cached_size, out);
        } else {
            std::memcpy(out, cached_buffer, cached_size);
            return out + cached_size;
        }
    }
#else
    char* write(char* out) const noexcept {
        std::memcpy(out, cached_buffer, cached_size);
        return out + cached_size;
    }
#endif
};

template <typename Catter,
          typename T,
          typename = std::enable_if_t<std::is_base_of_v<base_catter, Catter> &&
                                      std::is_floating_point_v<T>>>
constexpr auto operator<<(Catter c, T curr) noexcept {
    return c << floating_point_writer<T>{{}, std::move(curr), {}};
}

}  // namespace lazycat
