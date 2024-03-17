cmake_minimum_required(VERSION 3.2)

# set(SOURCE_FILES
#     division.h
#     division.cpp
# )

set(
    CORE_SOURCE_FILES
    "command.cpp"
    "row.cpp"
    "table.cpp"
    "dbfile.cpp"
    "btree.cpp"
)

add_library(core SHARED STATIC ${CORE_SOURCE_FILES})
target_include_directories(core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(core PRIVATE cxx_std_17)

# add_library(row SHARED STATIC "row.cpp")
# target_include_directories(row PRIVATE "./")

# add_library(table SHARED STATIC "table.cpp")
# target_include_directories(table PRIVATE "./")