#include <string>
#include <type_traits>
#include <utility>

// Define LAZYCAT_DANGEROUS_OPTIMIZATIONS to enable dangerous optimizations that are technically
// undefined behavior (default is off)

// __assume macro
#if defined(_MSC_VER)
#define LAZYCAT_ASSUME(cond) __assume(cond)
#elif defined(__clang__)
#define LAZYCAT_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#define LAZYCAT_ASSUME(cond) \
    if (!(cond)) __builtin_unreachable()
#else
#define LAZYCAT_ASSUME(cond)
#endif

// expands to 'constexpr' if std::string has constexpr member functions (needs <string>)
#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907
#define LAZYCAT_CONSTEXPR_STRING constexpr
#else
#define LAZYCAT_CONSTEXPR_STRING
#endif

#if defined(__cpp_lib_constexpr_string) && __cpp_lib_constexpr_string >= 201907 && \
    (!LAZYCAT_DANGEROUS_OPTIMIZATIONS ||                                           \
     (defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811))
#define LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC constexpr
#else
#define LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC
#endif

namespace lazycat {
namespace detail {
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
    // the following two functions are same as base, so that append works
    // static constexpr char_type* move(char_type* dest, const char_type*, std::size_t) noexcept;
    // static constexpr char_type* copy(char_type* dest, const char_type*, std::size_t) noexcept;
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
    LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC static S construct_default_init(size_t sz) {
#ifdef LAZYCAT_DANGEROUS_OPTIMIZATIONS
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811
        if (std::is_constant_evaluated()) {
            return S(sz, typename S::value_type{});
        } else {
            std::basic_string<char, noop_char_traits> ret(sz, typename S::value_type{});
            return reinterpret_cast<S&&>(std::move(ret));  // this is UB but works
        }
#else
        std::basic_string<char, noop_char_traits> ret(sz, typename S::value_type{});
        return reinterpret_cast<S&&>(std::move(ret));  // this is UB but works
#endif
#else
        return S(sz, typename S::value_type{});
#endif
    }
};
template <typename S>
struct construct_default_init_t<
    S,
    void_t<decltype(std::declval<S&>().__resize_default_init(std::declval<size_t>()))>> {
    LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC static S construct_default_init(size_t sz) {
        S s;
        s.__resize_default_init(sz);
        return s;
    }
};
template <typename S, typename = void>
struct append_default_init_t {
    LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC static char* append_default_init(S& s, size_t sz) {
        const size_t old_sz = s.size();
        LAZYCAT_ASSUME(old_sz <= old_sz + sz);
#ifdef LAZYCAT_DANGEROUS_OPTIMIZATIONS
#if defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811
        if (std::is_constant_evaluated()) {
            s.append(sz, char{});
        } else {
            reinterpret_cast<std::basic_string<char, noop_char_traits>&>(s).append(sz, char{});
        }
#else
        reinterpret_cast<std::basic_string<char, noop_char_traits>&>(s).append(sz, char{});
#endif
#else
        s.append(sz, char{});
#endif
        return s.data() + old_sz;
    }
};
template <typename S>
struct append_default_init_t<
    S,
    void_t<decltype(std::declval<S&>().__append_default_init(std::declval<size_t>()))>> {
    LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC static char* append_default_init(S& s, size_t sz) {
        const size_t old_sz = s.size();
        LAZYCAT_ASSUME(old_sz <= old_sz + sz);
        s.__append_default_init(sz);
        return s.data() + old_sz;
    }
};

LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC inline std::string construct_default_init(size_t sz) {
    return construct_default_init_t<std::string>::construct_default_init(sz);
}

LAZYCAT_CONSTEXPR_STRING_UTIL_MAGIC inline char* append_default_init(std::string& s, size_t sz) {
    return append_default_init_t<std::string>::append_default_init(s, sz);
}

}  // namespace detail
}  // namespace lazycat
