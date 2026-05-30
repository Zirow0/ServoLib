/**
 * @file board_config.h
 * @brief Конфігурація STM32F411CEU6 — 6-осьовий серво-контролер (Master)
 *
 * Підключення:
 *   ADC1 CH0-5    → PA0-PA5  — ACS712 датчики струму, моторів 0-5
 *
 *   TIM3 CH1 (PWM)→ PA6 (AF2) — Мотор 0 PWM
 *   TIM3 CH2 (PWM)→ PA7 (AF2) — Мотор 1 PWM
 *   TIM3 CH3 (PWM)→ PB0 (AF2) — Мотор 2 PWM
 *   TIM3 CH4 (PWM)→ PB1 (AF2) — Мотор 3 PWM
 *   TIM1 CH1 (PWM)→ PA8 (AF1) — Мотор 4 PWM  [потребує MOE в BDTR]
 *   TIM1 CH4 (PWM)→ PA11(AF1) — Мотор 5 PWM
 *
 *   DIR GPIO:  PB3-PB8        — Моторів 0-5
 *
 *   Гальма:    PB9, PB10, PA12, PA15, PB2, PC14  — Брейки 0-5
 *              LOW = гальмо увімкнено (fail-safe)
 *
 *   SPI2 master: NSS=PB12(SW GPIO), SCK=PB13, MISO=PB14, MOSI=PB15 (AF5)
 *                Mode 0, 6.25 MHz (APB1 50 MHz / 8) — до EncoderHub
 *
 *   USART1:    TX=PA9, RX=PA10 (AF7) — зв'язок з хостом
 *
 *   E-STOP:    PC15, GPIO_IN, pull-up, EXTI15 falling — кнопка на GND
 *
 *   TIM5 (32-bit): внутрішній µs лічильник
 *   SysTick:       1 kHz ms лічильник
 *   LED:           PC13 (активний LOW на BlackPill)
 *
 * Примітки:
 *   PB11 відсутній на UFQFPN48
 *   PA13/PA14 — SWD (не займати)
 *   PA9/PA10 — зайняті USART1; тому TIM1 CH2/CH3 недоступні
 *   TIM1 — advanced timer: Board_Init() встановлює BDTR.MOE до App-ініціалізації
 */

#ifndef SERVO_MASTER_BOARD_CONFIG_H
#define SERVO_MASTER_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "../../Inc/core.h"

/* ── Активні модулі ─────────────────────────────────────────────────────────*/
#define USE_MOTOR_PWM
#define USE_BRAKE
#define USE_HWD_ADC
#define USE_SENSOR_CURRENT
#define USE_SENSOR_ACS712
#define USE_HWD_SPI
#define USE_HWD_UART

/* ── Системний клок ─────────────────────────────────────────────────────────*/
#define SYSTEM_CORE_CLOCK   100000000U  /* HSE 25 MHz + PLL → 100 MHz       */
#define APB1_TIMER_CLOCK    100000000U  /* APB1=50 MHz, таймери APB1 × 2    */
/* TIM1 (APB2): APB2=100 MHz × 1 = 100 MHz — рівне APB1_TIMER_CLOCK       */

/* ── PWM: спільні параметри для всіх 6 моторів ─────────────────────────────*/
#define MOTOR_PWM_FREQ      20000U   /* 20 кГц                              */
#define MOTOR_PWM_PERIOD    999U     /* resolution = 1000 кроків            */

/* ── Мотор 0: TIM3_CH1 → PA6 (AF2) ─────────────────────────────────────── */
#define MOTOR0_PWM_TIMER     TIM3
#define MOTOR0_PWM_TIMER_RCC RCC_TIM3
#define MOTOR0_PWM_OC        TIM_OC1
#define MOTOR0_PWM_GPIO_PORT GPIOA
#define MOTOR0_PWM_GPIO_PIN  GPIO6
#define MOTOR0_PWM_GPIO_AF   GPIO_AF2
#define MOTOR0_DIR_GPIO_PORT GPIOB
#define MOTOR0_DIR_PIN       GPIO3

/* ── Мотор 1: TIM3_CH2 → PA7 (AF2) ─────────────────────────────────────── */
#define MOTOR1_PWM_TIMER     TIM3
#define MOTOR1_PWM_TIMER_RCC RCC_TIM3
#define MOTOR1_PWM_OC        TIM_OC2
#define MOTOR1_PWM_GPIO_PORT GPIOA
#define MOTOR1_PWM_GPIO_PIN  GPIO7
#define MOTOR1_PWM_GPIO_AF   GPIO_AF2
#define MOTOR1_DIR_GPIO_PORT GPIOB
#define MOTOR1_DIR_PIN       GPIO4

/* ── Мотор 2: TIM3_CH3 → PB0 (AF2) ─────────────────────────────────────── */
#define MOTOR2_PWM_TIMER     TIM3
#define MOTOR2_PWM_TIMER_RCC RCC_TIM3
#define MOTOR2_PWM_OC        TIM_OC3
#define MOTOR2_PWM_GPIO_PORT GPIOB
#define MOTOR2_PWM_GPIO_PIN  GPIO0
#define MOTOR2_PWM_GPIO_AF   GPIO_AF2
#define MOTOR2_DIR_GPIO_PORT GPIOB
#define MOTOR2_DIR_PIN       GPIO5

/* ── Мотор 3: TIM3_CH4 → PB1 (AF2) ─────────────────────────────────────── */
#define MOTOR3_PWM_TIMER     TIM3
#define MOTOR3_PWM_TIMER_RCC RCC_TIM3
#define MOTOR3_PWM_OC        TIM_OC4
#define MOTOR3_PWM_GPIO_PORT GPIOB
#define MOTOR3_PWM_GPIO_PIN  GPIO1
#define MOTOR3_PWM_GPIO_AF   GPIO_AF2
#define MOTOR3_DIR_GPIO_PORT GPIOB
#define MOTOR3_DIR_PIN       GPIO6

/* ── Мотор 4: TIM1_CH1 → PA8 (AF1) ─────────────────────────────────────── */
/* TIM1 — advanced timer: BDTR.MOE встановлюється у Board_Init()            */
#define MOTOR4_PWM_TIMER     TIM1
#define MOTOR4_PWM_TIMER_RCC RCC_TIM1
#define MOTOR4_PWM_OC        TIM_OC1
#define MOTOR4_PWM_GPIO_PORT GPIOA
#define MOTOR4_PWM_GPIO_PIN  GPIO8
#define MOTOR4_PWM_GPIO_AF   GPIO_AF1
#define MOTOR4_DIR_GPIO_PORT GPIOB
#define MOTOR4_DIR_PIN       GPIO7

/* ── Мотор 5: TIM1_CH4 → PA11 (AF1) ────────────────────────────────────── */
#define MOTOR5_PWM_TIMER     TIM1
#define MOTOR5_PWM_TIMER_RCC RCC_TIM1
#define MOTOR5_PWM_OC        TIM_OC4
#define MOTOR5_PWM_GPIO_PORT GPIOA
#define MOTOR5_PWM_GPIO_PIN  GPIO11
#define MOTOR5_PWM_GPIO_AF   GPIO_AF1
#define MOTOR5_DIR_GPIO_PORT GPIOB
#define MOTOR5_DIR_PIN       GPIO8

/* ── Гальма 0-5: GPIO OUT PP, LOW = гальмо увімкнено (fail-safe) ────────── */
#define BRAKE0_GPIO_PORT     GPIOB
#define BRAKE0_PIN           GPIO9

#define BRAKE1_GPIO_PORT     GPIOB
#define BRAKE1_PIN           GPIO10

#define BRAKE2_GPIO_PORT     GPIOA
#define BRAKE2_PIN           GPIO12

#define BRAKE3_GPIO_PORT     GPIOA
#define BRAKE3_PIN           GPIO15

/* BRAKE4: PB2 = BOOT1. Pull-down на платі BlackPill → при старті LOW.
 * Це природна fail-safe поведінка: гальмо увімкнено до ініціалізації App. */
#define BRAKE4_GPIO_PORT     GPIOB
#define BRAKE4_PIN           GPIO2

#define BRAKE5_GPIO_PORT     GPIOC
#define BRAKE5_PIN           GPIO14

/* ── ADC: ACS712 датчики струму (PA0-PA5, ADC1 CH0-5) ──────────────────── */
#ifdef USE_HWD_ADC
#include <libopencm3/stm32/adc.h>
#define CURRENT_ADC_PERIPH   ADC1
#define CURRENT_ADC_RCC      RCC_ADC1
#define CURRENT_ADC_VREF_V   3.3f

#define CURRENT0_ADC_GPIO_PORT  GPIOA
#define CURRENT0_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT0_ADC_GPIO_PIN   GPIO0
#define CURRENT0_ADC_CHANNEL    0U

#define CURRENT1_ADC_GPIO_PORT  GPIOA
#define CURRENT1_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT1_ADC_GPIO_PIN   GPIO1
#define CURRENT1_ADC_CHANNEL    1U

#define CURRENT2_ADC_GPIO_PORT  GPIOA
#define CURRENT2_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT2_ADC_GPIO_PIN   GPIO2
#define CURRENT2_ADC_CHANNEL    2U

#define CURRENT3_ADC_GPIO_PORT  GPIOA
#define CURRENT3_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT3_ADC_GPIO_PIN   GPIO3
#define CURRENT3_ADC_CHANNEL    3U

#define CURRENT4_ADC_GPIO_PORT  GPIOA
#define CURRENT4_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT4_ADC_GPIO_PIN   GPIO4
#define CURRENT4_ADC_CHANNEL    4U

#define CURRENT5_ADC_GPIO_PORT  GPIOA
#define CURRENT5_ADC_GPIO_RCC   RCC_GPIOA
#define CURRENT5_ADC_GPIO_PIN   GPIO5
#define CURRENT5_ADC_CHANNEL    5U
#endif /* USE_HWD_ADC */

/* ── SPI2 Master → EncoderHub (PB12-PB15, AF5) ──────────────────────────── */
/* Mode 0 (CPOL=0, CPHA=0), APB1(50 MHz)/8 = 6.25 MHz ≤ 10 MHz max         */
/* Software NSS: PB12 керується вручну як GPIO (не апаратний NSS)           */
#ifdef USE_HWD_SPI
#define EHUB_SPI             SPI2
#define EHUB_SPI_RCC         RCC_SPI2
#define EHUB_SPI_GPIO_PORT   GPIOB
#define EHUB_SPI_NSS_PORT    GPIOB
#define EHUB_SPI_NSS_PIN     GPIO12
#define EHUB_SPI_SCK_PIN     GPIO13
#define EHUB_SPI_MISO_PIN    GPIO14
#define EHUB_SPI_MOSI_PIN    GPIO15
#define EHUB_SPI_GPIO_AF     GPIO_AF5
#define EHUB_SPI_BAUDRATE    SPI_CR1_BAUDRATE_FPCLK_DIV_8  /* 6.25 MHz     */
#endif /* USE_HWD_SPI */

/* ── UART (USART1 → PA9/PA10, AF7) ─────────────────────────────────────── */
#ifdef USE_HWD_UART
#include <libopencm3/stm32/usart.h>
#define UART_DEBUG              USART1
#define UART_DEBUG_RCC          RCC_USART1
#define UART_DEBUG_GPIO_PORT    GPIOA
#define UART_DEBUG_TX_PIN       GPIO9
#define UART_DEBUG_RX_PIN       GPIO10
#define UART_DEBUG_GPIO_AF      GPIO_AF7
#define UART_DEBUG_BAUDRATE     1000000U
#endif /* USE_HWD_UART */

/* ── Async COMM (USART1 + DMA2, PA9/PA10, AF7) ───────────────────────────── */
/* Увімкнути через compile_definitions: USE_COMM_ASYNC                       */
/* Несумісно з USE_HWD_UART на одному USART                                  */
#ifdef USE_COMM_ASYNC
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#define COMM_USART              USART1
#define COMM_BAUD               1000000U
#define COMM_GPIO_PORT          GPIOA
#define COMM_TX_PIN             GPIO9
#define COMM_RX_PIN             GPIO10
/* DMA2: RX = Stream2 Ch4, TX = Stream7 Ch4 */
#define COMM_RX_DMA             DMA2
#define COMM_RX_DMA_STREAM      2U
#define COMM_RX_DMA_CHANNEL     4U
#define COMM_TX_DMA             DMA2
#define COMM_TX_DMA_STREAM      7U
#define COMM_TX_DMA_CHANNEL     4U
#define COMM_RX_BUF_SIZE        128U
#define COMM_TX_BUF_SIZE        96U
#define COMM_TX_QUEUE_LEN       4U
#endif /* USE_COMM_ASYNC */

/* ── E-STOP вхід (PC15, pull-up, active LOW) ───────────────────────────────*/
/* EXTI15 — falling edge; ISR встановлює g_estop_active = true              */
#define ESTOP_GPIO_PORT     GPIOC
#define ESTOP_PIN           GPIO15
#define ESTOP_EXTI          EXTI15
#define ESTOP_EXTI_IRQ      NVIC_EXTI15_10_IRQ

/* ── Microsecond Timer (TIM5, 32-bit) ───────────────────────────────────── */
#define MICROS_TIMER            TIM5
#define MICROS_TIMER_RCC        RCC_TIM5
#define MICROS_TIMER_PRESCALER  (100U - 1U)  /* 100 MHz / 100 = 1 MHz      */

/* ── GPIO: LED, SysTick ─────────────────────────────────────────────────── */
#define LED_GPIO_PORT       GPIOC
#define LED_PIN             GPIO13

#define SYSTICK_FREQ        1000U   /* 1 kHz = 1 ms                         */

/* ── Глобальний стан E-STOP ─────────────────────────────────────────────── */
extern volatile bool g_estop_active;

/* ── SysTick uptime ─────────────────────────────────────────────────────── */
extern volatile uint32_t g_uptime_ms;

/* ── Board API ──────────────────────────────────────────────────────────── */
Servo_Status_t Board_Init(void);

static inline bool Board_IsEstopActive(void) { return g_estop_active; }
static inline void Board_ClearEstop(void)     { g_estop_active = false; }

#ifdef __cplusplus
}
#endif

#endif /* SERVO_MASTER_BOARD_CONFIG_H */
