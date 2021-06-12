#include <cstring>
#include <string>
#include <string_view>

namespace lazycat_detail {
// should use nested references instead?
struct empty_cat;
template <typename Prev>
struct string_view_cat;
template <typename Prev>
struct char_cat;

template <typename Cat>
struct base_cat {
    operator std::string() const noexcept {
        const size_t sz = static_cast<const Cat&>(*this).size();
        std::string ret(sz, char{});
        static_cast<const Cat&>(*this).write(ret.data());
        return ret;
    }
    std::string build() const noexcept { return *this; }
    constexpr auto operator<<(std::string_view curr) const noexcept {
        return lazycat_detail::string_view_cat<Cat>{{}, static_cast<const Cat&>(*this), curr};
    }
    constexpr auto operator<<(char curr) const noexcept {
        return lazycat_detail::char_cat<Cat>{{}, static_cast<const Cat&>(*this), curr};
    }
    template <typename S>
    constexpr auto cat(S&& s) const noexcept {
        return *this << std::forward<S>(s);
    }
};

struct empty_cat : public base_cat<empty_cat> {
    constexpr static size_t size() noexcept { return 0; }
    constexpr static char* write(char* out) noexcept { return out; }
};

template <typename Prev>
struct string_view_cat : public base_cat<string_view_cat<Prev>> {
    Prev prev;
    std::string_view curr;
    constexpr size_t size() const noexcept { return prev.size() + curr.size(); }
    constexpr char* write(char* out) const noexcept {
        out = prev.write(out);
        std::memcpy(out, curr.data(), curr.size());
        return out + curr.size();
    }
};

template <typename Prev>
struct char_cat : public base_cat<char_cat<Prev>> {
    Prev prev;
    char curr;
    constexpr size_t size() const noexcept { return prev.size() + 1; }
    constexpr char* write(char* out) const noexcept {
        out = prev.write(out);
        *out++ = curr;
        return out;
    }
};
}  // namespace lazycat_detail

template <typename... Ss>
constexpr inline auto lazycat(Ss&&... ss) noexcept {
    return (lazycat_detail::empty_cat{} << ... << ss);
}
