# ─── STM32F411CEU6 — 6-осьовий серво-контролер (Master) ─────────────────────
#
# Ресурси:
#   PWM:     TIM3 CH1-4 (PA6,PA7,PB0,PB1) + TIM1 CH1,CH4 (PA8,PA11)
#   DIR:     PB3-PB8
#   Brakes:  PB9,PB10,PA12,PA15,PB2,PC14
#   ADC:     PA0-PA5 (ADC1 CH0-5) — ACS712 × 6
#   SPI2:    PB12-PB15 → EncoderHub (master, Mode 0, 6.25 MHz)
#   USART1:  PA9/PA10 (simple або async/DMA)
#   E-STOP:  PC15 (EXTI15 falling)
#   TIM5:    µs лічильник

set(DEVICE "stm32f411ceu6")

set(MCU_DIR   ${CMAKE_SOURCE_DIR}/MCU/STM32F411_libopencm3)
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Board/STM32F411_ServoMaster)

set(MCU_SRCS
    ${MCU_DIR}/hwd_gpio.c
    ${MCU_DIR}/hwd_timer.c
    ${MCU_DIR}/hwd_uart.c
    ${MCU_DIR}/hwd_pwm.c
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
