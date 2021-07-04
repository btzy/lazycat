#pragma once

#include <lazycat/lazycat_core.hpp>

// This file contains the writer for bool

namespace lazycat {

struct bool_writer : public base_writer {
    bool content;
    constexpr size_t size() const noexcept { return 1; }
    constexpr char* write(char* out) const noexcept {
        *out++ = content ? '1' : '0';
        return out;
    }
};

template <typename Catter, typename = std::enable_if_t<std::is_base_of_v<base_catter, Catter>>>
constexpr auto operator<<(Catter c, bool curr) noexcept {
    return c << bool_writer{{}, curr};
}

}  // namespace lazycat
