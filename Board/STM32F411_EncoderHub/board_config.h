/**
 * @file board_config.h
 * @brief Конфігурація STM32F411CEU6 — Encoder Hub
 *
 * Підключення:
 *   ENC0: A=PA0 (EXTI0),  B=PA1 (EXTI1)
 *   ENC1: A=PA2 (EXTI2),  B=PA3 (EXTI3)
 *   ENC2: A=PA4 (EXTI4),  B=PA5 (EXTI5)
 *   ENC3: A=PA6 (EXTI6),  B=PA7 (EXTI7)
 *   ENC4: A=PA8 (EXTI8),  B=PB9 (EXTI9)
 *   ENC5: A=PB10 (EXTI10),B=PA11 (EXTI11)
 *   SPI2 slave: NSS=PB12, SCK=PB13, MISO=PB14, MOSI=PB15 (AF5)
 *   DRDY out:   PB0
 *   USART1 debug: TX=PA9, RX=PA10 (AF7, 1 Мбод)
 *   TIM2 (32-bit): free-running µs counter
 *   TIM3: 10 kHz update interrupt (UKF trigger)
 */

#ifndef ENCODER_HUB_BOARD_CONFIG_H
#define ENCODER_HUB_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "core.h"

/* Активні модулі ------------------------------------------------------------*/
#define USE_ENCODER_HUB
#define USE_SENSOR_POSITION
#define USE_SENSOR_INCREMENTAL
#define USE_HWD_UART

/* Системний клок ------------------------------------------------------------*/
#define SYSTEM_CORE_CLOCK   100000000U  /* 100 MHz (HSE 25 MHz + PLL) */
#define APB1_TIMER_CLOCK    100000000U  /* ×2 від APB1=50 MHz */

/* Microsecond Timer (TIM2, 32-bit) ------------------------------------------*/
#define MICROS_TIMER            TIM2
#define MICROS_TIMER_RCC        RCC_TIM2
#define MICROS_TIMER_PRESCALER  (100U - 1U)  /* 100 MHz / 100 = 1 MHz */

/* Update Timer 10 kHz (TIM5) ------------------------------------------------*/
/* TIM5 обрано щоб не конфліктувати з tim2/tim3/tim4_isr драйвера енкодера */
#define UPDATE_TIMER            TIM5
#define UPDATE_TIMER_RCC        RCC_TIM5
#define UPDATE_TIMER_IRQ        NVIC_TIM5_IRQ
#define UPDATE_TIMER_PRESCALER  (0U)
#define UPDATE_TIMER_PERIOD     (9999U)  /* 100 MHz / 1 / 10000 = 10 kHz */

/* Encoders (EXTI-only mode: timer_base=0, gpio_af=0) ------------------------*/
/* Швидкість вимірюється через EXTI timestamp з TIM2 (Incremental_Encoder_IC_Handler).
 * timer_base=0 вмикає EXTI-only гілку у драйвері (без TIM IC). */
#define ENCODER_COUNT  6U

/* CPR (counts per revolution після X4) — однаковий для всіх 6; налаштувати під реальний енкодер */
#define HUB_ENCODER_CPR  4000U

/* ENC0: A=PA0 (EXTI0), B=PA1 (EXTI1) */
#define ENC0_GPIO_PORT_A    GPIOA
#define ENC0_GPIO_PIN_A     GPIO0
#define ENC0_GPIO_PORT_B    GPIOA
#define ENC0_GPIO_PIN_B     GPIO1
#define ENC0_GPIO_AF        0U
#define ENC0_TIMER_BASE     0U
#define ENC0_TIMER_RCC      0U
#define ENC0_CPR            HUB_ENCODER_CPR

/* ENC1: A=PA2 (EXTI2), B=PA3 (EXTI3) */
#define ENC1_GPIO_PORT_A    GPIOA
#define ENC1_GPIO_PIN_A     GPIO2
#define ENC1_GPIO_PORT_B    GPIOA
#define ENC1_GPIO_PIN_B     GPIO3
#define ENC1_GPIO_AF        0U
#define ENC1_TIMER_BASE     0U
#define ENC1_TIMER_RCC      0U
#define ENC1_CPR            HUB_ENCODER_CPR

/* ENC2: A=PA4 (EXTI4), B=PA5 (EXTI5) */
#define ENC2_GPIO_PORT_A    GPIOA
#define ENC2_GPIO_PIN_A     GPIO4
#define ENC2_GPIO_PORT_B    GPIOA
#define ENC2_GPIO_PIN_B     GPIO5
#define ENC2_GPIO_AF        0U
#define ENC2_TIMER_BASE     0U
#define ENC2_TIMER_RCC      0U
#define ENC2_CPR            HUB_ENCODER_CPR

/* ENC3: A=PA6 (EXTI6), B=PA7 (EXTI7) */
#define ENC3_GPIO_PORT_A    GPIOA
#define ENC3_GPIO_PIN_A     GPIO6
#define ENC3_GPIO_PORT_B    GPIOA
#define ENC3_GPIO_PIN_B     GPIO7
#define ENC3_GPIO_AF        0U
#define ENC3_TIMER_BASE     0U
#define ENC3_TIMER_RCC      0U
#define ENC3_CPR            HUB_ENCODER_CPR

/* ENC4: A=PA8 (EXTI8), B=PB9 (EXTI9) */
#define ENC4_GPIO_PORT_A    GPIOA
#define ENC4_GPIO_PIN_A     GPIO8
#define ENC4_GPIO_PORT_B    GPIOB
#define ENC4_GPIO_PIN_B     GPIO9
#define ENC4_GPIO_AF        0U
#define ENC4_TIMER_BASE     0U
#define ENC4_TIMER_RCC      0U
#define ENC4_CPR            HUB_ENCODER_CPR

/* ENC5: A=PB10 (EXTI10), B=PA11 (EXTI11) */
#define ENC5_GPIO_PORT_A    GPIOB
#define ENC5_GPIO_PIN_A     GPIO10
#define ENC5_GPIO_PORT_B    GPIOA
#define ENC5_GPIO_PIN_B     GPIO11
#define ENC5_GPIO_AF        0U
#define ENC5_TIMER_BASE     0U
#define ENC5_TIMER_RCC      0U
#define ENC5_CPR            HUB_ENCODER_CPR

/* SPI2 Slave (PB12-PB15, AF5) ----------------------------------------------*/
#define HUB_SPI             SPI2
#define HUB_SPI_RCC         RCC_SPI2
#define HUB_SPI_GPIO_PORT   GPIOB
#define HUB_SPI_NSS_PIN     GPIO12
#define HUB_SPI_SCK_PIN     GPIO13
#define HUB_SPI_MISO_PIN    GPIO14
#define HUB_SPI_MOSI_PIN    GPIO15
#define HUB_SPI_GPIO_AF     GPIO_AF5

/* DRDY output (PB0 → master) -----------------------------------------------*/
#define DRDY_GPIO_PORT      GPIOB
#define DRDY_PIN            GPIO0

/* UART debug (USART1 → PA9/PA10, AF7) --------------------------------------*/
#ifdef USE_HWD_UART
#include <libopencm3/stm32/usart.h>
#define UART_DEBUG              USART1
#define UART_DEBUG_RCC          RCC_USART1
#define UART_DEBUG_GPIO_PORT    GPIOA
#define UART_DEBUG_TX_PIN       GPIO9
#define UART_DEBUG_RX_PIN       GPIO10
#define UART_DEBUG_GPIO_AF      GPIO_AF7
#define UART_DEBUG_BAUDRATE     1000000U
#endif

/* LED -----------------------------------------------------------------------*/
#define LED_GPIO_PORT   GPIOC
#define LED_PIN         GPIO13

/* SysTick -------------------------------------------------------------------*/
#define SYSTICK_FREQ    1000U  /* 1 kHz = 1 ms */

extern volatile uint32_t g_uptime_ms;

/* Board API -----------------------------------------------------------------*/
Servo_Status_t Board_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_HUB_BOARD_CONFIG_H */
