# Відлагодження PWM двигуна
# Залежності: util + motor + board (PWM, GPIO, Timer)

stm32_add_executable(debug_motor
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_UTIL}
    ${SERVOLIB_MOTOR}
)
