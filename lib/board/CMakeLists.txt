add_library(lafvin_board STATIC
    src/board.c
    src/lcd_st7796.c
    src/touch_gt911.c
)

target_include_directories(lafvin_board PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(lafvin_board PUBLIC
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_pwm
    hardware_spi
)
