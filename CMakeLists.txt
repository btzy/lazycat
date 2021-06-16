cmake_minimum_required(VERSION 3.9)
project(lazycat)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)

include(CTest)

# If LAZYCAT_BUILD_TESTS is ON or OFF, use it to decide if tests should be built.
# Otherwise, use BUILD_TESTING to decide.
set(LAZYCAT_BUILD_TESTS DEFAULT CACHE STRING "Build tests for lazycat")
set_property(CACHE LAZYCAT_BUILD_TESTS PROPERTY STRINGS DEFAULT ON OFF)
set(LAZYCAT_REALLY_BUILD_TESTS ${LAZYCAT_BUILD_TESTS})
if(LAZYCAT_REALLY_BUILD_TESTS STREQUAL "DEFAULT")
  set(LAZYCAT_REALLY_BUILD_TESTS ${BUILD_TESTING})
endif()
if(${LAZYCAT_REALLY_BUILD_TESTS})
    add_subdirectory(vendor/Catch2)
    add_subdirectory(test)
endif()

option(LAZYCAT_BUILD_BENCHMARKS "Build benchmarks for lazycat" OFF)
if(${LAZYCAT_BUILD_BENCHMARKS})
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BUILD_TESTING OFF)
    # set(BENCHMARK_ENABLE_LTO ON)
    add_subdirectory(vendor/benchmark)
    add_subdirectory(vendor/abseil-cpp)
    add_subdirectory(vendor/fmt)
    add_subdirectory(benchmark)
endif()