cmake_minimum_required(VERSION 3.29)

project(detscat VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(detscat ${SRCS})

target_include_directories(
    detscat
    PRIVATE "${PROJECT_SOURCE_DIR}/include" 
)

target_compile_options(detscat PRIVATE -Wall -Wextra -pedantic)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(detscat PRIVATE -O0 -g)
endif()
