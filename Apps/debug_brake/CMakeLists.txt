# Відлагодження гальма
# Залежності: util + brake + board (GPIO, Timer)

stm32_add_executable(debug_brake
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_UTIL}
    ${SERVOLIB_BRAKE}
)
