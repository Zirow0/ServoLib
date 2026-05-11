# Базовий сервопривід (каскадний PID, без safety та trajectory)

stm32_add_executable(servo_basic
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
)
