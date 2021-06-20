add_executable(lazycat_benchmark "benchmark_string_cat.cpp" "benchmark_char_cat.cpp" "benchmark_string_append_realloc.cpp" "benchmark_string_append_norealloc.cpp")
add_executable(lazycat_benchmark_dangerous "benchmark_string_cat.cpp" "benchmark_char_cat.cpp" "benchmark_string_append_realloc.cpp" "benchmark_string_append_norealloc.cpp")

target_compile_definitions(lazycat_benchmark_dangerous PRIVATE LAZYCAT_DANGEROUS_OPTIMIZATIONS)

target_link_libraries(lazycat_benchmark PUBLIC lazycat benchmark::benchmark benchmark::benchmark_main absl::strings fmt::fmt-header-only)
target_link_libraries(lazycat_benchmark_dangerous PUBLIC lazycat benchmark::benchmark benchmark::benchmark_main absl::strings fmt::fmt-header-only)

add_test(lazycat_benchmark lazycat_benchmark)
add_test(lazycat_benchmark_dangerous lazycat_benchmark_dangerous)
