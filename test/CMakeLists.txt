add_executable(unit_test unit_test.cpp)

target_link_libraries(unit_test PUBLIC lazycat)
target_link_libraries(unit_test PUBLIC Catch2WithMain)

add_test(unit_test unit_test)
