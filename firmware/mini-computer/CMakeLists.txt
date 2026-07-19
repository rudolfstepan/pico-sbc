add_executable(lafvin_minicomputer
    src/main.c
    src/ui.c
)

target_include_directories(lafvin_minicomputer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(lafvin_minicomputer
    lafvin_board
)

pico_enable_stdio_usb(lafvin_minicomputer 1)
pico_enable_stdio_uart(lafvin_minicomputer 0)
pico_set_program_name(lafvin_minicomputer "LAFVIN Mini Computer")
pico_set_program_version(lafvin_minicomputer "1.0.0")
set_target_properties(lafvin_minicomputer PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
pico_add_extra_outputs(lafvin_minicomputer)
