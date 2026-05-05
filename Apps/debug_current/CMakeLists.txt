# Відлагодження датчика струму ACS712
# Залежності: util + current + board (ADC DMA, Timer, UART)

stm32_add_executable(debug_current
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_UTIL}
    ${SERVOLIB_CURRENT}
)
