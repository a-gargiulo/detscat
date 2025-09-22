cmake_minimum_required(VERSION 3.29)

project(detscat VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.c")

# add_executable(detscat ${SRCS})
add_executable(detscat 
    "${PROJECT_SOURCE_DIR}/main.c" 
    ${SRCS}
)

target_include_directories(
    detscat PRIVATE
    "${PROJECT_SOURCE_DIR}/include" 
    "${PROJECT_SOURCE_DIR}/lib/stb" 
)


if(APPLE)
    set(OpenMP_INCLUDE_DIR /opt/homebrew/opt/libomp/include)
    set(OpenMP_LIBRARY /opt/homebrew/opt/libomp/lib/libomp.dylib)

    target_include_directories(detscat PRIVATE ${OpenMP_INCLUDE_DIR})
    target_link_libraries(detscat PRIVATE ${OpenMP_LIBRARY})
else()
    find_package(OpenMP REQUIRED)
    if(OpenMP_C_FOUND)
        target_compile_options(detscat PRIVATE ${OpenMP_C_FLAGS})
        target_link_libraries(detscat PRIVATE ${OpenMP_C_LIBRARIES})
    endif()
endif()

target_compile_options(detscat PRIVATE -Wall -Wextra -pedantic -O0 -g)
