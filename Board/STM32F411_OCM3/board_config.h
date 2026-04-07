/**
 * @file board_config.h
 * @brief Конфігурація апаратного забезпечення для STM32F411CEU6 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * Платформо-специфічна конфігурація для STM32F411CEU6 (BlackPill)
 * з використанням libopencm3 замість STM32 HAL.
 *
 * Підключення:
 *   TIM3 CH1 (PWM)            → PA6  (AF2)
 *   PA7      (DIR)             → PA7  (GPIO OUT)
 *   TIM2 CH1 (Encoder A)      → PA0  (AF1)
 *   TIM2 CH2 (Encoder B)      → PA1  (AF1)
 *   Brake                     → PA8  (GPIO OUT)
 *   I2C1 SCL                  → PB6  (AF4)
 *   I2C1 SDA                  → PB7  (AF4)
 *   TIM5 (мікросекунди)       → внутрішній (без пінів)
 */

#ifndef SERVOCORE_BOARD_CONFIG_H
#define SERVOCORE_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "../../Inc/core.h"

/* Hardware Module Configuration ---------------------------------------------*/
#define USE_REAL_HARDWARE   1

/* Вибір драйверів */
#define USE_MOTOR_PWM       1
#define USE_BRAKE           1

#define USE_SENSOR_POSITION
#define USE_SENSOR_INCREMENTAL

/* SPI/I2C та відповідні датчики вимкнено */
// #define USE_HWD_SPI
// #define USE_SENSOR_AEAT9922
// #define USE_HWD_I2C
// #define USE_SENSOR_AS5600

#define USE_HWD_UART

/* System Configuration ------------------------------------------------------*/

/** @brief Частота системного ядра (Hz) — 100 MHz через HSE + PLL */
#define SYSTEM_CORE_CLOCK       100000000U

/** @brief Частота APB1 шини (Hz) — max 50 MHz для STM32F411 */
#define APB1_TIMER_CLOCK        100000000U  /**< ×2 від APB1 = 100 MHz для таймерів */

/** @brief Частота APB2 шини (Hz) */
#define APB2_TIMER_CLOCK        100000000U

/* PWM Configuration (TIM3) --------------------------------------------------*/

/** @brief Базова адреса таймера PWM */
#define MOTOR_PWM_TIMER         TIM3

/** @brief RCC для TIM3 */
#define MOTOR_PWM_TIMER_RCC     RCC_TIM3

/** @brief PWM канал 1 (прямий хід) — PA6, AF2 */
#define MOTOR_PWM_OC_FWD        TIM_OC1

/** @brief PWM канал 2 (зворотний хід) — PA7, AF2 */
#define MOTOR_PWM_OC_BWD        TIM_OC2

/** @brief Частота PWM (Hz) */
#define MOTOR_PWM_FREQ          1000U

/** @brief Кількість кроків роздільної здатності (0..999) */
#define MOTOR_PWM_PERIOD        999U

/* GPIO для TIM3 PWM */
#define MOTOR_PWM_GPIO_PORT     GPIOA
#define MOTOR_PWM_GPIO_RCC      RCC_GPIOA
#define MOTOR_PWM_GPIO_CH1      GPIO6   /**< PA6 → TIM3 CH1 */
#define MOTOR_PWM_GPIO_CH2      GPIO7   /**< PA7 → TIM3 CH2 */
#define MOTOR_PWM_GPIO_AF       GPIO_AF2

/* Incremental Encoder (TIM2, quadrature x4) ---------------------------------*/

/** @brief Базова адреса таймера-енкодера (32-bit) */
#define ENCODER_TIMER_BASE      TIM2

/** @brief RCC для TIM2 */
#define ENCODER_TIMER_RCC       RCC_TIM2

/** @brief GPIO канал A — PA0, AF1 (TIM2 CH1) */
#define ENCODER_GPIO_PORT_A     GPIOA
#define ENCODER_GPIO_PIN_A      GPIO0
#define ENCODER_GPIO_RCC_A      RCC_GPIOA

/** @brief GPIO канал B — PA1, AF1 (TIM2 CH2) */
#define ENCODER_GPIO_PORT_B     GPIOA
#define ENCODER_GPIO_PIN_B      GPIO1
#define ENCODER_GPIO_RCC_B      RCC_GPIOA

/** @brief Alternate Function для TIM2 */
#define ENCODER_GPIO_AF         GPIO_AF1

/** @brief Кількість відліків за оберт після x4 квадратури */
#define ENCODER_CPR             4000U

/* Microsecond Timer (TIM5) --------------------------------------------------*/

/** @brief Базова адреса таймера мікросекунд (32-bit) */
#define MICROS_TIMER            TIM5

/** @brief RCC для TIM5 */
#define MICROS_TIMER_RCC        RCC_TIM5

/** @brief Prescaler для 1 MHz тактування (100 MHz / 100 = 1 MHz) */
#define MICROS_TIMER_PRESCALER  (100U - 1U)

/* GPIO Configuration --------------------------------------------------------*/

/** @brief Brake GPIO — PA8 */
#define BRAKE_CTRL_GPIO_PORT    GPIOA
#define BRAKE_CTRL_GPIO_RCC     RCC_GPIOA
#define BRAKE_CTRL_PIN          GPIO8

/** @brief LED GPIO — PC13 */
#define LED_GPIO_PORT           GPIOC
#define LED_GPIO_RCC            RCC_GPIOC
#define LED_PIN                 GPIO13

/* UART Configuration (USART1) -----------------------------------------------*/

#ifdef USE_HWD_UART

#include <libopencm3/stm32/usart.h>

/** @brief Базова адреса USART для відлагодження */
#define UART_DEBUG              USART1

/** @brief RCC для USART1 */
#define UART_DEBUG_RCC          RCC_USART1

/** @brief GPIO порт для PA9/PA10 */
#define UART_DEBUG_GPIO_PORT    GPIOA
#define UART_DEBUG_GPIO_RCC     RCC_GPIOA

/** @brief PA9 → USART1 TX */
#define UART_DEBUG_TX_PIN       GPIO9

/** @brief PA10 → USART1 RX */
#define UART_DEBUG_RX_PIN       GPIO10

/** @brief Alternate function для USART1 */
#define UART_DEBUG_GPIO_AF      GPIO_AF7

/** @brief Швидкість UART (бод) */
#define UART_DEBUG_BAUDRATE     115200U

#endif /* USE_HWD_UART */

/* I2C Configuration (вимкнено) ----------------------------------------------*/
#ifdef USE_HWD_I2C

/** @brief Базова адреса I2C */
#define SENSOR_I2C              I2C1

/** @brief RCC для I2C1 */
#define SENSOR_I2C_RCC          RCC_I2C1

/** @brief GPIO для I2C1 */
#define SENSOR_I2C_GPIO_PORT    GPIOB
#define SENSOR_I2C_GPIO_RCC     RCC_GPIOB
#define SENSOR_I2C_SCL          GPIO6   /**< PB6 → I2C1 SCL */
#define SENSOR_I2C_SDA          GPIO7   /**< PB7 → I2C1 SDA */
#define SENSOR_I2C_GPIO_AF      GPIO_AF4

/** @brief APB clock для розрахунку I2C швидкості (MHz) */
#define SENSOR_I2C_APB_MHZ      50U

#endif /* USE_HWD_I2C */

/* SysTick -------------------------------------------------------------------*/

/** @brief Частота SysTick (Hz) — 1 kHz = 1 ms на тік */
#define SYSTICK_FREQ            1000U

/** @brief Глобальний лічильник мілісекунд (оновлюється в sys_tick_handler) */
extern volatile uint32_t g_uptime_ms;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація апаратного забезпечення плати
 *
 * Налаштовує системний клок, RCC, GPIO alternate functions,
 * TIM5 (мікросекунди), SPI1, SysTick.
 * Викликається один раз до використання будь-якого HWD драйвера.
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Board_Init(void);

/**
 * @brief Деініціалізація апаратного забезпечення
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Board_DeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_BOARD_CONFIG_H */
