#pragma once

#include <cstring>
#include <lazycat/util.hpp>
#include <string>
#include <string_view>

namespace lazycat {

struct base_catter {};
struct base_writer {};

// A writer must have these functions:
// size_t size();
// char* write(char* out);
// constexprness, constness, and noexceptness is optional, but good to have.
// size() will be called once, and then write() will be called once, so it is possible to generate
// some cached value in size() (see lazycat_integral.hpp).  However, try to keep constructors and
// destructors trivial.

// stuff for cat():

template <typename Prev, typename Writer>
struct combined_catter;

template <typename Catter>
struct catter : public base_catter {
    // Note: Somehow this function being non-noexcept makes it noticeably slower than Abseil on
    // MacOS Clang, but we do want allocation failure to throw an exception like usual.
    LAZYCAT_CONSTEXPR_STRING operator std::string() const {
        const size_t sz = static_cast<const Catter&>(*this).size();
        std::string ret = detail::construct_default_init(sz);
        static_cast<const Catter&>(*this).write(ret.data());
        return ret;
    }
    LAZYCAT_CONSTEXPR_STRING std::string build() const { return *this; }
    template <typename Writer, typename = std::enable_if_t<std::is_base_of_v<base_writer, Writer>>>
    constexpr auto operator<<(Writer writer) const noexcept {
        return combined_catter<Catter, Writer>{{}, static_cast<const Catter&>(*this), writer};
    }
    template <typename S>
    constexpr auto cat(S&& s) const noexcept {
        return *this << std::forward<S>(s);
    }
};

struct empty_catter : public catter<empty_catter> {
    constexpr static size_t size() noexcept { return 0; }
    constexpr static char* write(char* out) noexcept { return out; }
};

template <typename Prev, typename Writer>
struct combined_catter : public catter<combined_catter<Prev, Writer>> {
    Prev prev;
    Writer writer;
    constexpr size_t size() const noexcept { return prev.size() + writer.size(); }
    constexpr char* write(char* out) const noexcept { return writer.write(prev.write(out)); }
};

// stuff for append():

template <typename Prev, typename Writer>
struct combined_appender;

template <typename Appender>
struct appender : public base_catter {
    constexpr void build() const { static_cast<const Appender&>(*this).resize_and_write(0); }
    template <typename Writer, typename = std::enable_if_t<std::is_base_of_v<base_writer, Writer>>>
    constexpr auto operator<<(Writer writer) const noexcept {
        return combined_appender<Appender, Writer>{{}, static_cast<const Appender&>(*this), writer};
    }
    template <typename S>
    constexpr auto append(S&& s) const noexcept {
        return *this << std::forward<S>(s);
    }
};

struct empty_appender : public appender<empty_appender> {
    std::string& content;
    // Resizes the root string such that later we still can write `sz` bytes, then write everything
    // we need to write
    LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC char* resize_and_write(size_t sz) const {
        return detail::append_default_init(content, sz);
    }
};  // namespace lazycat

template <typename Prev, typename Writer>
struct combined_appender : public appender<combined_appender<Prev, Writer>> {
    Prev prev;
    Writer writer;
    constexpr char* resize_and_write(size_t sz) const {
        return writer.write(prev.resize_and_write(writer.size() + sz));
    }
};

// helpers for each type:

struct string_view_writer : public base_writer {
    std::string_view content;
    constexpr size_t size() const noexcept { return content.size(); }
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811
    constexpr char* write(char* out) const noexcept {
        if (std::is_constant_evaluated()) {
            return std::copy(content.begin(), content.end(), out);
        } else {
            std::memcpy(out, content.data(), content.size());
            return out + content.size();
        }
    }
#else
    char* write(char* out) const noexcept {
        std::memcpy(out, content.data(), content.size());
        return out + content.size();
    }
#endif
};

template <typename Catter, typename = std::enable_if_t<std::is_base_of_v<base_catter, Catter>>>
constexpr auto operator<<(Catter c, std::string_view curr) noexcept {
    return c << string_view_writer{{}, curr};
}

struct char_writer : public base_writer {
    char content;
    constexpr size_t size() const noexcept { return 1; }
    constexpr char* write(char* out) const noexcept {
        *out++ = content;
        return out;
    }
};

template <typename Catter, typename = std::enable_if_t<std::is_base_of_v<base_catter, Catter>>>
[[nodiscard]] constexpr auto operator<<(Catter c, char curr) noexcept {
    return c << char_writer{{}, curr};
}

// main interface:

template <typename... Ss>
[[nodiscard]] constexpr inline auto cat(Ss&&... ss) noexcept {
    return (empty_catter{} << ... << ss);
}

template <typename... Ss>
[[nodiscard]] constexpr inline auto append(std::string& str, Ss&&... ss) noexcept {
    return (empty_appender{{}, str} << ... << ss);
}

}  // namespace lazycat
