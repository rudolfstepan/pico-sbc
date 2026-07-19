cmake_minimum_required(VERSION 3.16)

project(lafvin_pico_sbc_host_tests C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CTest)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(CALCULATOR_SRC ${PROJECT_ROOT}/firmware/scientific-calculator/src)
set(TINYEXPR_SRC ${PROJECT_ROOT}/third_party/tinyexpr)

function(enable_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
        target_link_libraries(${target} PRIVATE m)
    endif()
endfunction()

add_executable(calculator_engine_tests
    calculator/test_engine.c
    ${CALCULATOR_SRC}/calculator_engine.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(calculator_engine_tests PRIVATE
    ${CALCULATOR_SRC}
    ${TINYEXPR_SRC}
)
target_compile_definitions(calculator_engine_tests PRIVATE TE_POW_FROM_RIGHT)
enable_project_warnings(calculator_engine_tests)
add_test(NAME calculator_engine COMMAND calculator_engine_tests)

add_executable(programmer_engine_tests
    calculator/test_programmer.c
    ${CALCULATOR_SRC}/programmer_engine.c
)
target_include_directories(programmer_engine_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(programmer_engine_tests)
add_test(NAME programmer_engine COMMAND programmer_engine_tests)

add_executable(number_format_tests
    calculator/test_number_formats.c
    ${CALCULATOR_SRC}/number_formats.c
)
target_include_directories(number_format_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(number_format_tests)
add_test(NAME number_formats COMMAND number_format_tests)
