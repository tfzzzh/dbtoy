cmake_minimum_required(VERSION 3.2)

# set(SOURCE_FILES
#     division.h
#     division.cpp
# )

add_library(cmd SHARED STATIC "command.cpp")
target_include_directories(cmd PRIVATE "./")

add_library(row SHARED STATIC "row.cpp")
target_include_directories(row PRIVATE "./")

add_library(table SHARED STATIC "table.cpp")
target_include_directories(table PRIVATE "./")