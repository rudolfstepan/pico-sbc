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
    ${CALCULATOR_SRC}/calculator_symbols.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(calculator_engine_tests PRIVATE
    ${CALCULATOR_SRC}
    ${TINYEXPR_SRC}
)
target_compile_definitions(calculator_engine_tests PRIVATE TE_POW_FROM_RIGHT)
enable_project_warnings(calculator_engine_tests)
add_test(NAME calculator_engine COMMAND calculator_engine_tests)

add_executable(basic_engine_tests
    calculator/test_basic_engine.c
    ${CALCULATOR_SRC}/basic_engine.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(basic_engine_tests PRIVATE
    ${CALCULATOR_SRC}
    ${TINYEXPR_SRC}
)
target_compile_definitions(basic_engine_tests PRIVATE TE_POW_FROM_RIGHT)
enable_project_warnings(basic_engine_tests)
add_test(NAME basic_engine COMMAND basic_engine_tests)

add_executable(calculator_program_tests
    calculator/test_calculator_program.c
    calculator/mock_lcd.c
    ${CALCULATOR_SRC}/basic_engine.c
    ${CALCULATOR_SRC}/calculator_program.c
    ${CALCULATOR_SRC}/expression_editor.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(calculator_program_tests PRIVATE
    ${CALCULATOR_SRC}
    ${PROJECT_ROOT}/lib/board/include
    ${TINYEXPR_SRC}
    calculator
)
target_compile_definitions(calculator_program_tests PRIVATE TE_POW_FROM_RIGHT)
enable_project_warnings(calculator_program_tests)
add_test(NAME calculator_program COMMAND calculator_program_tests)

add_executable(programmer_engine_tests
    calculator/test_programmer.c
    ${CALCULATOR_SRC}/number_formats.c
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
    ${CALCULATOR_SRC}/calculator_symbols.c
    ${CALCULATOR_SRC}/calculator_graph.c
    ${CALCULATOR_SRC}/calculator_keymaps.c
    ${CALCULATOR_SRC}/calculator_widgets.c
    ${CALCULATOR_SRC}/graph_model.c
    ${CALCULATOR_SRC}/numerical_analysis.c
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

add_executable(numerical_analysis_tests
    calculator/test_numerical_analysis.c
    ${CALCULATOR_SRC}/calculation_status.c
    ${CALCULATOR_SRC}/numerical_analysis.c
)
target_include_directories(numerical_analysis_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(numerical_analysis_tests)
add_test(NAME numerical_analysis COMMAND numerical_analysis_tests)

add_executable(calculator_symbols_tests
    calculator/test_calculator_symbols.c
    ${CALCULATOR_SRC}/calculator_symbols.c
)
target_include_directories(calculator_symbols_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_symbols_tests)
add_test(NAME calculator_symbols COMMAND calculator_symbols_tests)

add_executable(calculator_pages_tests
    calculator/test_calculator_pages.c
    calculator/mock_lcd.c
    ${CALCULATOR_SRC}/calculator_complex.c
    ${CALCULATOR_SRC}/calculator_keymaps.c
    ${CALCULATOR_SRC}/calculator_logic.c
    ${CALCULATOR_SRC}/calculator_statistics.c
    ${CALCULATOR_SRC}/calculator_units.c
    ${CALCULATOR_SRC}/calculator_pages.c
    ${CALCULATOR_SRC}/calculator_symbols.c
    ${CALCULATOR_SRC}/calculator_widgets.c
    ${CALCULATOR_SRC}/complex_engine.c
    ${CALCULATOR_SRC}/expression_editor.c
    ${CALCULATOR_SRC}/number_formats.c
    ${CALCULATOR_SRC}/logic_engine.c
    ${CALCULATOR_SRC}/programmer_engine.c
    ${CALCULATOR_SRC}/statistics_engine.c
    ${CALCULATOR_SRC}/unit_engine.c
)
target_include_directories(calculator_pages_tests PRIVATE
    ${CALCULATOR_SRC}
    ${PROJECT_ROOT}/lib/board/include
    calculator
)
enable_project_warnings(calculator_pages_tests)
add_test(NAME calculator_pages COMMAND calculator_pages_tests)

add_executable(calculator_persistence_tests
    calculator/test_calculator_persistence.c
    ${CALCULATOR_SRC}/basic_engine.c
    ${CALCULATOR_SRC}/calculator_persistence.c
    ${CALCULATOR_SRC}/calculator_symbols.c
    ${CALCULATOR_SRC}/calculation_status.c
    ${CALCULATOR_SRC}/graph_model.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(calculator_persistence_tests PRIVATE
    ${CALCULATOR_SRC}
    ${TINYEXPR_SRC}
)
target_compile_definitions(calculator_persistence_tests PRIVATE
    TE_POW_FROM_RIGHT
)
enable_project_warnings(calculator_persistence_tests)
add_test(NAME calculator_persistence COMMAND calculator_persistence_tests)

add_executable(logic_engine_tests
    calculator/test_logic_engine.c
    ${CALCULATOR_SRC}/logic_engine.c
)
target_include_directories(logic_engine_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(logic_engine_tests)
add_test(NAME logic_engine COMMAND logic_engine_tests)

add_executable(calculator_logic_tests
    calculator/test_calculator_logic.c
    ${CALCULATOR_SRC}/calculator_logic.c
    ${CALCULATOR_SRC}/expression_editor.c
    ${CALCULATOR_SRC}/logic_engine.c
)
target_include_directories(calculator_logic_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_logic_tests)
add_test(NAME calculator_logic COMMAND calculator_logic_tests)

add_executable(unit_engine_tests
    calculator/test_unit_engine.c
    ${CALCULATOR_SRC}/unit_engine.c
)
target_include_directories(unit_engine_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(unit_engine_tests)
add_test(NAME unit_engine COMMAND unit_engine_tests)

add_executable(calculator_units_tests
    calculator/test_calculator_units.c
    ${CALCULATOR_SRC}/calculator_units.c
    ${CALCULATOR_SRC}/unit_engine.c
)
target_include_directories(calculator_units_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_units_tests)
add_test(NAME calculator_units COMMAND calculator_units_tests)

add_executable(complex_engine_tests
    calculator/test_complex_engine.c
    ${CALCULATOR_SRC}/complex_engine.c
)
target_include_directories(complex_engine_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(complex_engine_tests)
add_test(NAME complex_engine COMMAND complex_engine_tests)

add_executable(calculator_complex_tests
    calculator/test_calculator_complex.c
    ${CALCULATOR_SRC}/calculator_complex.c
    ${CALCULATOR_SRC}/complex_engine.c
    ${CALCULATOR_SRC}/expression_editor.c
)
target_include_directories(calculator_complex_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_complex_tests)
add_test(NAME calculator_complex COMMAND calculator_complex_tests)

add_executable(statistics_engine_tests
    calculator/test_statistics_engine.c
    ${CALCULATOR_SRC}/statistics_engine.c
)
target_include_directories(statistics_engine_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(statistics_engine_tests)
add_test(NAME statistics_engine COMMAND statistics_engine_tests)

add_executable(calculator_statistics_tests
    calculator/test_calculator_statistics.c
    ${CALCULATOR_SRC}/calculator_statistics.c
    ${CALCULATOR_SRC}/expression_editor.c
    ${CALCULATOR_SRC}/statistics_engine.c
)
target_include_directories(calculator_statistics_tests PRIVATE ${CALCULATOR_SRC})
enable_project_warnings(calculator_statistics_tests)
add_test(NAME calculator_statistics COMMAND calculator_statistics_tests)

add_executable(calculator_usb_protocol_tests
    calculator/test_calculator_usb_protocol.c
    ${CALCULATOR_SRC}/basic_engine.c
    ${CALCULATOR_SRC}/calculation_status.c
    ${CALCULATOR_SRC}/calculator_engine.c
    ${CALCULATOR_SRC}/calculator_symbols.c
    ${CALCULATOR_SRC}/calculator_usb_protocol.c
    ${CALCULATOR_SRC}/expression_editor.c
    ${CALCULATOR_SRC}/graph_model.c
    ${CALCULATOR_SRC}/statistics_engine.c
    ${TINYEXPR_SRC}/tinyexpr.c
)
target_include_directories(calculator_usb_protocol_tests PRIVATE
    ${CALCULATOR_SRC}
    ${TINYEXPR_SRC}
)
target_compile_definitions(calculator_usb_protocol_tests PRIVATE
    TE_POW_FROM_RIGHT
)
enable_project_warnings(calculator_usb_protocol_tests)
add_test(NAME calculator_usb_protocol COMMAND calculator_usb_protocol_tests)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME pico_calc_cli
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_LIST_DIR}/tools/test_pico_calc_cli.py
    )
    add_test(NAME pico_calc_gui_model
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_LIST_DIR}/tools/test_pico_calc_gui_model.py
    )
endif()
