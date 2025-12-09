/**
 * @file board_config.h
 * @brief Конфігурація апаратного забезпечення для STM32F411CEU6
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить визначення пінів, таймерів та периферії для
 * конкретної плати STM32F411CEU6 (BlackPill).
 */

#ifndef SERVOCORE_BOARD_CONFIG_H
#define SERVOCORE_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "../../Inc/core.h"  /* Використовуємо типи з core.h */

/* Hardware Module Configuration ---------------------------------------------*/
/* #define USE_HWD_I2C */    /* I2C вимкнено - AS5600 не використовується */
#define USE_HWD_SPI		     /* SPI увімкнено - для AEAT-9922 */

#define USE_SENSOR_AEAT9922  /* увімкнено AEAT-9922 */

/* Forward declarations ------------------------------------------------------*/
typedef struct __SPI_HandleTypeDef SPI_HandleTypeDef;

/* PWM Configuration ---------------------------------------------------------*/

/**
 * @brief PWM таймер для двигуна (TIM3)
 *
 * Конфігурація CubeMX:
 * - Prescaler: 99 (для 1 MHz тактування)
 * - Period: 999 (для 1 kHz PWM)
 * - Mode: PWM Generation CH1
 */
extern TIM_HandleTypeDef htim3;

/** @brief PWM таймер для мотора */
#define MOTOR_PWM_TIMER              htim3

/** @brief PWM канал 1 (прямий хід) - PA6 */
#define MOTOR_PWM_CHANNEL_FWD        TIM_CHANNEL_1

/** @brief PWM канал 2 (зворотний хід) - PA7 */
#define MOTOR_PWM_CHANNEL_BWD        TIM_CHANNEL_2

/** @brief Частота PWM (Hz) */
#define MOTOR_PWM_FREQ               1000

/** @brief Період таймера для PWM */
#define MOTOR_PWM_PERIOD             999

/* SPI Configuration ---------------------------------------------------------*/

/**
 * @brief SPI1 для енкодера AEAT-9922
 *
 * Конфігурація CubeMX:
 * - Mode: Full-Duplex Master
 * - Frame Format: Motorola
 * - Data Size: 8 Bits
 * - First Bit: MSB First
 * - Prescaler: 8 (12.5 MHz при 100 MHz APB2)
 * - CPOL: Low, CPHA: 2 Edge
 * - SCK: PA5
 * - MISO: PA6
 * - MOSI: PA7
 * - CS: PA4 (GPIO)
 */
extern SPI_HandleTypeDef hspi1;

/** @brief SPI для енкодера AEAT-9922 */
#define ENCODER_SPI                  hspi1

/** @brief GPIO порт для CS енкодера */
#define ENCODER_CS_GPIO_PORT         GPIOA
#define ENCODER_CS_PIN               GPIO_PIN_4

/** @brief GPIO порт для MSEL енкодера */
#define ENCODER_MSEL_GPIO_PORT       GPIOB
#define ENCODER_MSEL_PIN             GPIO_PIN_0

/* Encoder Timer Configuration -----------------------------------------------*/

/**
 * @brief Таймер для інкрементального виходу AEAT-9922 (опціонально)
 *
 * Конфігурація CubeMX:
 * - Mode: Encoder Mode
 * - Encoder Mode: TI1 and TI2
 * - Period: 0xFFFFFFFF (32-біт)
 * - CH1: PA0, CH2: PA1
 */
#ifdef USE_ENCODER_INCREMENTAL
extern TIM_HandleTypeDef htim2;
#define ENCODER_TIMER                htim2
#endif

/* Timer Configuration -------------------------------------------------------*/

/**
 * @brief Таймер для мікросекундного відліку (TIM5)
 *
 * Конфігурація CubeMX:
 * - Prescaler: 99 (для 1 MHz тактування)
 * - Period: 0xFFFFFFFF (максимум для 32-біт таймера)
 * - Mode: Base Timer
 */
extern TIM_HandleTypeDef htim5;

/** @brief Таймер для мікросекундного відліку */
#define MICROS_TIMER                 htim5

/* UART Configuration --------------------------------------------------------*/

/**
 * @brief UART для налагодження (опціонально)
 *
 * Конфігурація CubeMX:
 * - Baudrate: 115200
 * - TX: PA9
 * - RX: PA10
 */
#ifdef USE_DEBUG_UART
extern UART_HandleTypeDef huart1;
#define DEBUG_UART                   huart1
#endif

/* GPIO Configuration --------------------------------------------------------*/

/** @brief GPIO порт для LED індикації */
#define LED_GPIO_PORT                GPIOC
#define LED_PIN                      GPIO_PIN_13

/** @brief GPIO для enable драйвера мотора (опціонально) */
#define MOTOR_ENABLE_GPIO_PORT       GPIOA
#define MOTOR_ENABLE_PIN             GPIO_PIN_0

/** @brief GPIO для керування гальмами (опціонально) */
#define BRAKE_CTRL_GPIO_Port         GPIOA
#define BRAKE_CTRL_Pin               GPIO_PIN_8

/* System Configuration ------------------------------------------------------*/

/** @brief Частота системного таймера (Hz) */
#define SYSTEM_CORE_CLOCK            100000000U  /* 100 MHz */

/** @brief Частота APB1 шини (для TIM3) */
#define APB1_TIMER_CLOCK             100000000U  /* 100 MHz */

/** @brief Частота APB2 шини */
#define APB2_TIMER_CLOCK             100000000U  /* 100 MHz */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація апаратного забезпечення плати
 *
 * Викликається один раз при старті програми для налаштування всіх периферійних пристроїв
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
