add_library(lazycat INTERFACE
  "lazycat/lazycat.hpp"
  "lazycat/lazycat_core.hpp"
  "lazycat/util.hpp"
  )

target_include_directories(lazycat INTERFACE .)
