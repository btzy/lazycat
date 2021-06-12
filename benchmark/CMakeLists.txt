add_executable(lazycat_benchmark benchmark.cpp)

target_link_libraries(lazycat_benchmark PUBLIC lazycat benchmark::benchmark absl::strings)

add_test(lazycat_benchmark lazycat_benchmark)
