# ─── STM32F411CEU6 + libopencm3 ──────────────────────────────────────────────

set(DEVICE "stm32f411ceu6")

set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Board/STM32F411_OCM3)

set(BOARD_SRCS
    ${BOARD_DIR}/board.c
    ${BOARD_DIR}/hwd_gpio.c
    ${BOARD_DIR}/hwd_timer.c
    ${BOARD_DIR}/hwd_uart.c
    ${BOARD_DIR}/hwd_pwm.c
    ${BOARD_DIR}/hwd_i2c.c
    ${BOARD_DIR}/hwd_spi.c
    ${BOARD_DIR}/hwd_adc.c
)
