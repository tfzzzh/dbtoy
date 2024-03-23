cmake_minimum_required(VERSION 3.2)
project(db_toy_test)

add_subdirectory(lib/googletest)

enable_testing()

add_executable(
  db_test
  "src/start_tests.cpp"
  "src/btree_node_tests.cpp"
  "src/btreepager_tests.cpp"
  "src/btree_logic_tests.cpp"
)
target_link_libraries(
  db_test
  GTest::gtest_main
  core
)

target_include_directories(
    db_test
    PRIVATE
    lib/googletest/googletest/include
    ../src
)
target_compile_features(db_test PRIVATE cxx_std_17)

include(GoogleTest)
gtest_discover_tests(db_test)

# include_directories(${DIVISION_HEADERS_DIR})
# include_directories(lib/googletest/googletest/include)

# set(SOURCE_FILES main.cpp src/divider_tests.cpp)

# add_executable(divider_tests ${SOURCE_FILES})
# target_link_libraries(divider_tests division gtest)
# install(TARGETS divider_tests DESTINATION bin)

