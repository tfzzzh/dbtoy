cmake_minimum_required(VERSION 3.2)
project(db_toy_test)

add_subdirectory(lib/googletest)

enable_testing()

add_executable(
  hello_test
  "src/start_tests.cpp"
  "src/btree_tests.cpp"
)
target_link_libraries(
  hello_test
  GTest::gtest_main
  core
)

target_include_directories(
    hello_test
    PRIVATE
    lib/googletest/googletest/include
    ../src
)

include(GoogleTest)
gtest_discover_tests(hello_test)

# include_directories(${DIVISION_HEADERS_DIR})
# include_directories(lib/googletest/googletest/include)

# set(SOURCE_FILES main.cpp src/divider_tests.cpp)

# add_executable(divider_tests ${SOURCE_FILES})
# target_link_libraries(divider_tests division gtest)
# install(TARGETS divider_tests DESTINATION bin)

