#include <cstring>
#include <lazycat/util.hpp>
#include <string>
#include <string_view>

namespace lazycat {

struct base_catter {};
struct base_writer {};

template <typename Prev, typename Writer>
struct combined_catter;

template <typename Catter>
struct catter : public base_catter {
    operator std::string() const noexcept {
        const size_t sz = static_cast<const Catter&>(*this).size();
        std::string ret = detail::construct_default_init(sz);
        static_cast<const Catter&>(*this).write(ret.data());
        return ret;
    }
    std::string build() const noexcept { return *this; }
    template <typename Writer, typename = std::enable_if_t<std::is_base_of_v<base_writer, Writer>>>
    constexpr auto operator<<(Writer writer) const noexcept {
        return combined_catter<Catter, Writer>{{{}}, static_cast<const Catter&>(*this), writer};
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

struct string_view_writer : public base_writer {
    std::string_view content;
    constexpr size_t size() const noexcept { return content.size(); }
    constexpr char* write(char* out) const noexcept {
        if (std::is_constant_evaluated()) {
            return std::copy(content.begin(), content.end(), out);
        } else {
            std::memcpy(out, content.data(), content.size());
            return out + content.size();
        }
    }
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
constexpr auto operator<<(Catter c, char curr) noexcept {
    return c << char_writer{{}, curr};
}

template <typename... Ss>
constexpr inline auto cat(Ss&&... ss) noexcept {
    return (empty_catter{} << ... << ss);
}
}  // namespace lazycat
