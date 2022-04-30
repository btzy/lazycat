add_library(lazycat INTERFACE
  "lazycat/lazycat.hpp"
  "lazycat/lazycat_core.hpp"
  "lazycat/util.hpp"
  "lazycat/lazycat_integral.hpp"
  "lazycat/lazycat_bool.hpp"
 "lazycat/lazycat_floating_point.hpp")

target_include_directories(lazycat INTERFACE .)
