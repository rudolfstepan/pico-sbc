cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(lafvin_pico_sbc_firmware C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/third_party/tinyexpr/tinyexpr.c")
    message(FATAL_ERROR
        "TinyExpr is missing. Run: git submodule update --init --recursive")
endif()

pico_sdk_init()

add_subdirectory(lib/board)
add_subdirectory(firmware/mini-computer)
add_subdirectory(firmware/scientific-calculator)
