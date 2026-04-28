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

