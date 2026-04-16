/**
 * @file hwd_timer.c
 * @brief Реалізація HWD Timer для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізація таймерної абстракції через libopencm3:
 *   - SysTick ISR → g_uptime_ms (мілісекунди)
 *   - TIM5 counter → мікросекунди (32-bit, 1 MHz)
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_timer.h"
#include "./board_config.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

/* Private variables ---------------------------------------------------------*/

/**
 * @brief Лічильник мілісекунд з моменту старту.
 *
 * Інкрементується в sys_tick_handler() кожну 1 мс.
 * volatile — запобігає оптимізації компілятора при busy-wait.
 */
volatile uint32_t g_uptime_ms = 0;

/* SysTick interrupt handler -------------------------------------------------*/

/**
 * @brief Обробник переривання SysTick.
 *
 * Викликається автоматично кожну 1 мс (налаштовується в Board_Init).
 * Ім'я зафіксоване в libopencm3 як weak symbol у vector table.
 */
void sys_tick_handler(void)
{
    g_uptime_ms++;
}

/* Exported functions --------------------------------------------------------*/

uint32_t HWD_Timer_GetMillis(void)
{
    return g_uptime_ms;
}

uint32_t HWD_Timer_GetMicros(void)
{
    /* TIM5 — 32-bit таймер, prescaler=99, clock=100MHz → 1 тік = 1 мкс */
    return timer_get_counter(MICROS_TIMER);
}

void HWD_Timer_DelayMs(uint32_t ms)
{
    uint32_t start = g_uptime_ms;

    /* Правильна обробка переповнення uint32_t */
    while ((g_uptime_ms - start) < ms) {
        /* busy-wait */
    }
}

void HWD_Timer_DelayUs(uint32_t us)
{
    uint32_t start = HWD_Timer_GetMicros();

    /* TIM5 — 32-bit, переповнення через ~71 хвилин → безпечна арифметика */
    while ((HWD_Timer_GetMicros() - start) < us) {
        /* busy-wait */
    }
}

bool HWD_Timer_IsElapsed(uint32_t start, uint32_t interval)
{
    return ((g_uptime_ms - start) >= interval);
}

uint32_t HWD_Timer_GetElapsed(uint32_t start, uint32_t end)
{
    return (end - start);
}

/* Encoder timer (quadrature mode) -------------------------------------------*/

Servo_Status_t HWD_Timer_EncoderInit(HWD_Encoder_Handle_t* handle,
                                      const HWD_Encoder_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Тактування таймера та GPIO */
    rcc_periph_clock_enable(config->rcc_timer);
    rcc_periph_clock_enable(config->rcc_gpio_a);
    if (config->rcc_gpio_b != config->rcc_gpio_a) {
        rcc_periph_clock_enable(config->rcc_gpio_b);
    }

    /* GPIO канал A — AF вхід з підтяжкою вверх */
    gpio_mode_setup(config->gpio_port_a, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                    config->gpio_pin_a);
    gpio_set_af(config->gpio_port_a, config->gpio_af, config->gpio_pin_a);

    /* GPIO канал B — AF вхід з підтяжкою вверх */
    gpio_mode_setup(config->gpio_port_b, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                    config->gpio_pin_b);
    gpio_set_af(config->gpio_port_b, config->gpio_af, config->gpio_pin_b);

    /* Зупинити лічильник перед конфігурацією */
    timer_disable_counter(config->timer_base);
    timer_set_prescaler(config->timer_base, 0);

    /* ARR = 0xFFFFFFFF для 32-bit TIM2/TIM5, 0xFFFF для 16-bit */
    timer_set_period(config->timer_base, 0xFFFFFFFFU);

    /* Прив'язка входів: CH1 → TI1, CH2 → TI2 */
    timer_ic_set_input(config->timer_base, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(config->timer_base, TIM_IC2, TIM_IC_IN_TI2);

    /* Полярність фронту (інверсія = лічить по падаючому фронту) */
    timer_ic_set_polarity(config->timer_base, TIM_IC1,
        config->invert_a ? TIM_IC_FALLING : TIM_IC_RISING);
    timer_ic_set_polarity(config->timer_base, TIM_IC2,
        config->invert_b ? TIM_IC_FALLING : TIM_IC_RISING);

    /* Encoder mode 3 — x4 квадратура (обидва фронти обох каналів) */
    timer_slave_set_mode(config->timer_base, TIM_SMCR_SMS_EM3);

    /* Скидання лічильника та запуск */
    timer_set_counter(config->timer_base, 0);
    
    timer_ic_set_filter(TIM3, TIM_IC1, TIM_IC_OFF);
    timer_ic_set_filter(TIM3, TIM_IC2, TIM_IC_OFF);

    timer_enable_counter(config->timer_base);

    handle->timer_base = config->timer_base;

    return SERVO_OK;
}

Servo_Status_t HWD_Timer_EncoderDeInit(HWD_Encoder_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    timer_disable_counter(handle->timer_base);
    handle->timer_base = 0;

    return SERVO_OK;
}

int32_t HWD_Timer_EncoderRead(const HWD_Encoder_Handle_t* handle)
{
    return (int32_t)timer_get_counter(handle->timer_base);
}

void HWD_Timer_EncoderReset(HWD_Encoder_Handle_t* handle)
{
    timer_set_counter(handle->timer_base, 0);
}
