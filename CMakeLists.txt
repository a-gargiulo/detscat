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
    "${PROJECT_SOURCE_DIR}/lib/fftw/include"
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


if(WIN32)
    # Path to the FFTW DLL
    set(FFTW_LIB_NAME "fftw3-3")
    set(FFTW_DLL "${PROJECT_SOURCE_DIR}/lib/fftw/libfftw3-3.dll")

    # Copy the DLL to the executable output directory after building
    add_custom_command(TARGET detscat POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FFTW_DLL}"
            $<TARGET_FILE_DIR:detscat>
        COMMENT "Copying FFTW DLL to build output directory"
    )
else()
    set(FFTW_LIB_NAME "fftw3")
endif()


target_link_directories(detscat
    PRIVATE "${PROJECT_SOURCE_DIR}/lib/fftw")

target_link_libraries(detscat
    PRIVATE ${FFTW_LIB_NAME})

target_compile_options(detscat PRIVATE -Wall -Wextra -pedantic -O0 -g)
