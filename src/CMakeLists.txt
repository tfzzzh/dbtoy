cmake_minimum_required(VERSION 3.2)
project(dbtoy)

# add_subdirectory(division)
# set(SOURCE_FILES main.cpp)

add_subdirectory("command")
add_executable(dbtoy main.cpp)


target_link_libraries(dbtoy cmd row table)
target_include_directories(dbtoy PUBLIC ${PROJECT_SOURCE_DIR})
# target_include_directories(dbtoy PUBLIC "./command")
