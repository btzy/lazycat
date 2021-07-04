add_library(lazycat INTERFACE
  "lazycat/lazycat.hpp"
  "lazycat/lazycat_core.hpp"
  "lazycat/util.hpp"
  "lazycat/lazycat_integral.hpp"
)

target_include_directories(lazycat INTERFACE .)
