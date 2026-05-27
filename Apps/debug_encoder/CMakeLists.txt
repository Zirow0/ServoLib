# Відлагодження датчика положення
# Залежності: util + position + board (GPIO, Timer)

stm32_add_executable(debug_encoder
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_UTIL}
    ${SERVOLIB_POSITION}
)

target_include_directories(debug_encoder PRIVATE
    ${CMAKE_SOURCE_DIR}/Src
)

target_link_libraries(debug_encoder PRIVATE ukf_mcu)
