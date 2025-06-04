cmake_minimum_required(VERSION 3.29)

project(detscat VERSION 1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build")

file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(detscat ${SRCS})

target_include_directories(
    detscat
    PUBLIC "${PROJECT_SOURCE_DIR}/include" 
)
