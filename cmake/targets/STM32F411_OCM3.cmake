# ─── STM32F411CEU6 + libopencm3 ──────────────────────────────────────────────

set(DEVICE "stm32f411ceu6")

set(MCU_DIR   ${CMAKE_SOURCE_DIR}/MCU/STM32F411_libopencm3)
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Board/STM32F411_OCM3)

set(MCU_SRCS
    ${MCU_DIR}/hwd_gpio.c
    ${MCU_DIR}/hwd_timer.c
    ${MCU_DIR}/hwd_uart.c
    ${MCU_DIR}/hwd_pwm.c
    ${MCU_DIR}/hwd_i2c.c
    ${MCU_DIR}/hwd_spi.c
    ${MCU_DIR}/hwd_adc.c
)

set(MCU_ASYNC_SRCS
    ${MCU_DIR}/hwd_uart_async.c
    ${MCU_DIR}/hwd_crc32.c
)

set(BOARD_SRCS
    ${BOARD_DIR}/board.c
    ${MCU_SRCS}
)
