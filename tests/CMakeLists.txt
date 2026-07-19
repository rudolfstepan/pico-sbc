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
    ${CALCULATOR_SRC}/calculation_status.c
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

add_executable(expression_editor_tests
    calculator/test_expression_editor.c
    ${CALCULATOR_SRC}/expression_editor.c
)
target_include_directories(expression_editor_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(expression_editor_tests)
add_test(NAME expression_editor COMMAND expression_editor_tests)

add_executable(navigation_tests
    calculator/test_navigation.c
    ${CALCULATOR_SRC}/calculator_navigation.c
)
target_include_directories(navigation_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(navigation_tests)
add_test(NAME navigation COMMAND navigation_tests)

add_executable(keymap_tests
    calculator/test_keymaps.c
    ${CALCULATOR_SRC}/calculator_keymaps.c
)
target_include_directories(keymap_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(keymap_tests)
add_test(NAME keymaps COMMAND keymap_tests)

add_executable(calculation_status_tests
    calculator/test_calculation_status.c
    ${CALCULATOR_SRC}/calculation_status.c
)
target_include_directories(calculation_status_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculation_status_tests)
add_test(NAME calculation_status COMMAND calculation_status_tests)

add_executable(calculator_list_tests
    calculator/test_calculator_list.c
    ${CALCULATOR_SRC}/calculator_list.c
)
target_include_directories(calculator_list_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_list_tests)
add_test(NAME calculator_list COMMAND calculator_list_tests)

add_executable(calculator_dialog_tests
    calculator/test_calculator_dialog.c
    ${CALCULATOR_SRC}/calculator_dialog.c
)
target_include_directories(calculator_dialog_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_dialog_tests)
add_test(NAME calculator_dialog COMMAND calculator_dialog_tests)

add_executable(graph_model_tests
    calculator/test_graph_model.c
    ${CALCULATOR_SRC}/calculation_status.c
    ${CALCULATOR_SRC}/graph_model.c
)
target_include_directories(graph_model_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(graph_model_tests)
add_test(NAME graph_model COMMAND graph_model_tests)

add_executable(graph_renderer_tests
    calculator/test_graph_renderer.c
    calculator/mock_lcd.c
    ${CALCULATOR_SRC}/calculation_status.c
    ${CALCULATOR_SRC}/calculator_engine.c
    ${CALCULATOR_SRC}/calculator_graph.c
    ${CALCULATOR_SRC}/calculator_keymaps.c
    ${CALCULATOR_SRC}/calculator_widgets.c
    ${CALCULATOR_SRC}/graph_model.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(graph_renderer_tests PRIVATE
    ${CALCULATOR_SRC}
    ${PROJECT_ROOT}/lib/board/include
    ${TINYEXPR_SRC}
    calculator
)
target_compile_definitions(graph_renderer_tests PRIVATE TE_POW_FROM_RIGHT)
enable_project_warnings(graph_renderer_tests)
add_test(NAME graph_renderer COMMAND graph_renderer_tests)
