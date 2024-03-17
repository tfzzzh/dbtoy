cmake_minimum_required(VERSION 3.2)

project(dbtoy)

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2")

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

include_directories(${DIVISIBLE_INSTALL_INCLUDE_DIR})
include_directories(${DIVISION_HEADERS_DIR})

add_subdirectory(src)
add_subdirectory(test)


