add_executable(lafvin_scientific_calculator
    src/main.c
    src/calculation_status.c
    src/calculator_dialog.c
    src/calculator_engine.c
    src/calculator_graph.c
    src/calculator_keymaps.c
    src/calculator_list.c
    src/calculator_navigation.c
    src/calculator_pages.c
    src/calculator_persistence.c
    src/calculator_storage.c
    src/calculator_symbols.c
    src/calculator_widgets.c
    src/number_formats.c
    src/programmer_engine.c
    src/expression_editor.c
    src/graph_model.c
    src/numerical_analysis.c
    src/calculator_ui.c
    ${PROJECT_SOURCE_DIR}/third_party/tinyexpr/tinyexpr.c
)

target_include_directories(lafvin_scientific_calculator PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PROJECT_SOURCE_DIR}/third_party/tinyexpr
)

target_compile_definitions(lafvin_scientific_calculator PRIVATE
    TE_POW_FROM_RIGHT
)

target_link_libraries(lafvin_scientific_calculator
    lafvin_board
    hardware_flash
    hardware_sync
    m
)

pico_add_linker_script_override_path(
    lafvin_scientific_calculator
    ${CMAKE_CURRENT_LIST_DIR}/linker
    FILES pico_flash_region.ld
)

pico_enable_stdio_usb(lafvin_scientific_calculator 1)
pico_enable_stdio_uart(lafvin_scientific_calculator 0)
pico_set_program_name(lafvin_scientific_calculator "LAFVIN Scientific Calculator")
pico_set_program_version(lafvin_scientific_calculator "1.0.0")
set_target_properties(lafvin_scientific_calculator PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
pico_add_extra_outputs(lafvin_scientific_calculator)
