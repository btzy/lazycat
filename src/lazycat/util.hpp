#include <string>
#include <type_traits>
#include <utility>

// Define LAZYCAT_DANGEROUS_OPTIMIZATIONS to enable dangerous optimizations that are technically
// undefined behavior (default is off)

namespace lazycat_detail {
// helper void_t
#if defined(__cpp_lib_void_t) && __cpp_lib_void_t >= 201411
using std::void_t;
#else
template <typename... Ts>
struct make_void {
    typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;
#endif

#ifdef LAZYCAT_DANGEROUS_OPTIMIZATIONS
// A char_trait for char that where all chars (except EOF) are equal to one another.
struct noop_char_traits : public std::char_traits<char> {
    static constexpr void assign(char_type&, const char_type&) noexcept {}
    static constexpr char_type* assign(char_type* p, std::size_t, char_type) noexcept { return p; }
    static constexpr bool eq(char_type, char_type) noexcept { return true; }
    static constexpr bool lt(char_type, char_type) noexcept { return false; }
    static constexpr char_type* move(char_type* dest, const char_type*, std::size_t) noexcept {
        return dest;
    }
    static constexpr char_type* copy(char_type* dest, const char_type*, std::size_t) noexcept {
        return dest;
    }
    static constexpr int compare(const char_type*, const char_type*, std::size_t) noexcept {
        return 0;
    }
    // static constexpr std::size_t length( const char_type* s ); // can't be defined
    static constexpr const char_type* find(const char_type* p,
                                           std::size_t count,
                                           const char_type&) {
        return count > 0 ? p : nullptr;
    }
    // static constexpr char_type to_char_type( int_type c ) noexcept; // same as base
    // static constexpr int_type to_int_type(char_type c) noexcept; // same as base
    static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept {
        return c1 == c2 || (c1 != eof() && c2 != eof());
    }
    // static constexpr int_type eof() noexcept; // same as base
    static constexpr int_type not_eof(int_type e) noexcept { return e != eof() ? e : 0; }
};
#endif

// Like s.resize(sz) but without writing to the new chars.
// libc++ has __resize_default_init so we can do an optimisation.
template <typename S, typename = void>
struct construct_default_init_t {
    static S construct_default_init(size_t sz) noexcept {
#ifdef LAZYCAT_DANGEROUS_OPTIMIZATIONS
        std::basic_string<char, noop_char_traits> ret(sz, typename S::value_type{});
        return reinterpret_cast<S&&>(std::move(ret));  // this is UB but works
#else
        return S(sz, typename S::value_type{});
#endif
    }
};
template <typename S>
struct construct_default_init_t<
    S,
    void_t<decltype(std::declval<S&>().__resize_default_init(std::declval<size_t>()))>> {
    static S construct_default_init(size_t sz) noexcept {
        S s;
        s.__resize_default_init(sz);
        return s;
    }
};

inline std::string construct_default_init(size_t sz) noexcept {
    return construct_default_init_t<std::string>::construct_default_init(sz);
}
}  // namespace lazycat_detail
