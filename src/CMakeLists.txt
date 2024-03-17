cmake_minimum_required(VERSION 3.2)
project(dbtoy)

# add_subdirectory(division)
# set(SOURCE_FILES main.cpp)

add_subdirectory("core")
add_executable(dbtoy main.cpp)


target_link_libraries(dbtoy core)
target_include_directories(dbtoy PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_features(dbtoy PRIVATE cxx_std_17)
# target_include_directories(dbtoy PUBLIC "./command")
