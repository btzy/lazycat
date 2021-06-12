cmake_minimum_required(VERSION 3.2)
project(lazycat)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(src)

include(CTest)
if(${BUILD_TESTING})
    add_subdirectory(Catch2)
    add_subdirectory(test)
endif()
