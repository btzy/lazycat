set(SOURCE_FILES
  "unit_test.cpp"
  "integral_test.cpp"
  "bool_test.cpp"
)

add_executable(unit_test ${SOURCE_FILES})
add_executable(unit_test_dangerous ${SOURCE_FILES})

target_compile_definitions(unit_test_dangerous PRIVATE LAZYCAT_DANGEROUS_OPTIMIZATIONS)

target_link_libraries(unit_test PUBLIC lazycat Catch2WithMain)
target_link_libraries(unit_test_dangerous PUBLIC lazycat Catch2WithMain)

add_test(unit_test unit_test)
add_test(unit_test_dangerous unit_test_dangerous)
