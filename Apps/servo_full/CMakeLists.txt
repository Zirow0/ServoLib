# Повний сервопривід (всі підсистеми)

stm32_add_executable(servo_full
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
)
