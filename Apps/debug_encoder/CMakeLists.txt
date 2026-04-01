# Відлагодження датчика положення AEAT-9922
# Залежності: util + position + board (SPI, GPIO, Timer)

stm32_add_executable(debug_encoder
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_UTIL}
    ${SERVOLIB_POSITION}
)
