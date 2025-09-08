cmake_minimum_required(VERSION 3.29)

project(detscat VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.c")

# add_executable(detscat ${SRCS})
add_executable(detscat 
    "${PROJECT_SOURCE_DIR}/main.c" 
    "${PROJECT_SOURCE_DIR}/src/detscat_core.c" 
    "${PROJECT_SOURCE_DIR}/src/detscat_error.c" 
    # "${PROJECT_SOURCE_DIR}/src/detscat_parser.c" 
    # "${PROJECT_SOURCE_DIR}/src/detscat_str.c" 
    # "${PROJECT_SOURCE_DIR}/src/detscat_math.c" 
    "${PROJECT_SOURCE_DIR}/src/detscat_log.c" 
    # "${PROJECT_SOURCE_DIR}/src/detscat_cfg.c" 
    # "${PROJECT_SOURCE_DIR}/src/detscat_prt.c" 
)


target_include_directories(
    detscat
    PRIVATE "${PROJECT_SOURCE_DIR}/include" 
)

find_package(OpenMP REQUIRED)
if(OpenMP_C_FOUND)
    target_compile_options(detscat PRIVATE ${OpenMP_C_FLAGS})
    target_link_libraries(detscat PRIVATE ${OpenMP_C_LIBRARIES})
endif()

target_compile_options(detscat PRIVATE -Wall -Wextra -pedantic)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(detscat PRIVATE -O0 -g)
endif()
