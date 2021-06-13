add_executable(lazycat_benchmark "benchmark_string.cpp" "benchmark_char.cpp")
add_executable(lazycat_benchmark_dangerous "benchmark_string.cpp" "benchmark_char.cpp")

target_compile_definitions(lazycat_benchmark_dangerous PRIVATE LAZYCAT_DANGEROUS_OPTIMIZATIONS)

target_link_libraries(lazycat_benchmark PUBLIC lazycat benchmark::benchmark benchmark::benchmark_main absl::strings)
target_link_libraries(lazycat_benchmark_dangerous PUBLIC lazycat benchmark::benchmark benchmark::benchmark_main absl::strings)

add_test(lazycat_benchmark lazycat_benchmark)
add_test(lazycat_benchmark_dangerous lazycat_benchmark_dangerous)
