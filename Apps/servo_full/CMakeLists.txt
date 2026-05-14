# Повний сервопривід (всі підсистеми)

stm32_add_executable(servo_full
    main.c
    ${BOARD_SRCS}
    ${SERVOLIB_ALL}
)

target_link_libraries(servo_full PRIVATE ukf_mcu)
