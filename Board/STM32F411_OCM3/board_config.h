/**
 * @file board_config.h
 * @brief Конфігурація апаратного забезпечення STM32F411CEU6 (BlackPill)
 *
 * Підключення:
 *   TIM3 CH1 (PWM)   → PA6  (AF2)
 *   PA7      (DIR)   → PA7  (GPIO OUT)
 *   TIM2     (ENC)   → PA0/PA1 (AF1)
 *   Brake            → PA8  (GPIO OUT)
 *   I2C1 SCL/SDA     → PB6/PB7 (AF4)
 *   USART1 TX/RX     → PA9/PA10 (AF7)
 *   TIM5 (мкс)       → внутрішній
 */

#ifndef SERVOCORE_BOARD_CONFIG_H
#define SERVOCORE_BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "../../Inc/core.h"

/* Активні модулі ------------------------------------------------------------*/
#define USE_MOTOR_PWM
#define USE_BRAKE

#define USE_SENSOR_POSITION
#define USE_SENSOR_INCREMENTAL

// #define USE_HWD_SPI
// #define USE_HWD_I2C
// #define USE_SENSOR_AS5600

// #define USE_HWD_ADC
// #define USE_SENSOR_CURRENT
// #define USE_SENSOR_ACS712

#define USE_HWD_UART

/* Системний клок ------------------------------------------------------------*/
#define SYSTEM_CORE_CLOCK   100000000U  /* 100 MHz (HSE 25 MHz + PLL) */
#define APB1_TIMER_CLOCK    100000000U  /* ×2 від APB1=50 MHz */

/* PWM (TIM3 CH1 → PA6, AF2) ------------------------------------------------*/
#define MOTOR_PWM_TIMER         TIM3
#define MOTOR_PWM_TIMER_RCC     RCC_TIM3
#define MOTOR_PWM_OC_FWD        TIM_OC1
#define MOTOR_PWM_FREQ          20000U
#define MOTOR_PWM_PERIOD        999U
#define MOTOR_PWM_GPIO_PORT     GPIOA
#define MOTOR_PWM_GPIO_CH1      GPIO6
#define MOTOR_PWM_GPIO_AF       GPIO_AF2

/* DIR (PA7) -----------------------------------------------------------------*/
#define MOTOR_DIR_GPIO_PORT     GPIOA
#define MOTOR_DIR_PIN           GPIO7

/* Incremental Encoder (TIM2 → PA0/PA1, AF1) ---------------------------------*/
#define ENCODER_TIMER_BASE      TIM2
#define ENCODER_TIMER_RCC       RCC_TIM2
#define ENCODER_GPIO_PORT_A     GPIOA
#define ENCODER_GPIO_PIN_A      GPIO0
#define ENCODER_GPIO_RCC_A      RCC_GPIOA
#define ENCODER_GPIO_PORT_B     GPIOA
#define ENCODER_GPIO_PIN_B      GPIO1
#define ENCODER_GPIO_RCC_B      RCC_GPIOA
#define ENCODER_GPIO_AF         GPIO_AF1
#define ENCODER_CPR             4000U

/* Microsecond Timer (TIM5) --------------------------------------------------*/
#define MICROS_TIMER            TIM5
#define MICROS_TIMER_RCC        RCC_TIM5
#define MICROS_TIMER_PRESCALER  (100U - 1U)  /* 100 MHz / 100 = 1 MHz */

/* GPIO ----------------------------------------------------------------------*/
#define BRAKE_CTRL_GPIO_PORT    GPIOA
#define BRAKE_CTRL_PIN          GPIO8

#define LED_GPIO_PORT           GPIOC
#define LED_PIN                 GPIO13

/* UART (USART1 → PA9/PA10, AF7) --------------------------------------------*/
#ifdef USE_HWD_UART
#include <libopencm3/stm32/usart.h>
#define UART_DEBUG              USART1
#define UART_DEBUG_RCC          RCC_USART1
#define UART_DEBUG_GPIO_PORT    GPIOA
#define UART_DEBUG_TX_PIN       GPIO9
#define UART_DEBUG_RX_PIN       GPIO10
#define UART_DEBUG_GPIO_AF      GPIO_AF7
#define UART_DEBUG_BAUDRATE     115200U
#endif

/* ADC (ADC1 → PA4, CH4) ----------------------------------------------------*/
#ifdef USE_HWD_ADC
#include <libopencm3/stm32/adc.h>
#define CURRENT_ADC_PERIPH      ADC1
#define CURRENT_ADC_RCC         RCC_ADC1
#define CURRENT_ADC_GPIO_PORT   GPIOA
#define CURRENT_ADC_GPIO_RCC    RCC_GPIOA
#define CURRENT_ADC_GPIO_PIN    GPIO4
#define CURRENT_ADC_CHANNEL     4U
#define CURRENT_ADC_VREF_V      3.3f
#endif

/* I2C (I2C1 → PB6/PB7, AF4) -----------------------------------------------*/
#ifdef USE_HWD_I2C
#define SENSOR_I2C              I2C1
#define SENSOR_I2C_RCC          RCC_I2C1
#define SENSOR_I2C_GPIO_PORT    GPIOB
#define SENSOR_I2C_GPIO_RCC     RCC_GPIOB
#define SENSOR_I2C_SCL          GPIO6
#define SENSOR_I2C_SDA          GPIO7
#define SENSOR_I2C_GPIO_AF      GPIO_AF4
#define SENSOR_I2C_APB_MHZ      50U
#endif

/* SysTick -------------------------------------------------------------------*/
#define SYSTICK_FREQ            1000U  /* 1 kHz = 1 ms */

extern volatile uint32_t g_uptime_ms;

/* Board API -----------------------------------------------------------------*/
Servo_Status_t Board_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_BOARD_CONFIG_H */
