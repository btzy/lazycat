#include <string>
#include <type_traits>

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

// Like s.resize(sz) but without writing to the new chars.
// libc++ has __resize_default_init so we can do an optimisation.
template <typename S, typename = void>
struct construct_default_init_t {
    static S construct_default_init(size_t sz) noexcept { return S(sz, typename S::value_type{}); }
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
