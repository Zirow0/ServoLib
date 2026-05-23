# ─── STM32F411CEU6 + libopencm3 — Encoder Hub ────────────────────────────────

set(DEVICE "stm32f411ceu6")

set(MCU_DIR   ${CMAKE_SOURCE_DIR}/MCU/STM32F411_libopencm3)
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Board/STM32F411_EncoderHub)

set(MCU_SRCS
    ${MCU_DIR}/hwd_gpio.c
    ${MCU_DIR}/hwd_timer.c
    ${MCU_DIR}/hwd_uart.c
)

set(BOARD_SRCS
    ${BOARD_DIR}/board.c
    ${MCU_SRCS}
)
