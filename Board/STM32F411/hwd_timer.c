/**
 * @file hwd_timer.c
 * @brief Реалізація HWD Timer для STM32F411
 * @author ServoCore Team
 * @date 2025
 *
 * Платформо-специфічна реалізація таймерів для STM32F4xx.
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_timer.h"
#include "./board_config.h"

/* Private variables ---------------------------------------------------------*/

/** @brief Початковий час для мікросекундного таймера */
// UNUSED: static uint32_t micros_timer_start = 0;

/* Exported functions --------------------------------------------------------*/

uint32_t HWD_Timer_GetMillis(void)
{
    // Використовуємо системний тік HAL (налаштований на 1 мс)
    return HAL_GetTick();
}

uint32_t HWD_Timer_GetMicros(void)
{
    // Використовуємо TIM5 для мікросекунд
    // TIM5 - 32-bit таймер з prescaler=99 дає 1 MHz (1 us на тік)
    return __HAL_TIM_GET_COUNTER(&MICROS_TIMER);
}

void HWD_Timer_DelayMs(uint32_t ms)
{
    // Використовуємо стандартну HAL затримку
    HAL_Delay(ms);
}

void HWD_Timer_DelayUs(uint32_t us)
{
    // Для мікросекундної затримки використовуємо TIM5
    uint32_t start = HWD_Timer_GetMicros();

    // Очікування з урахуванням переповнення 32-bit таймера
    while ((HWD_Timer_GetMicros() - start) < us) {
        // Busy wait
    }
}

bool HWD_Timer_IsElapsed(uint32_t start, uint32_t interval)
{
    uint32_t current = HWD_Timer_GetMillis();

    // Правильна обробка переповнення
    uint32_t elapsed = current - start;

    return (elapsed >= interval);
}

uint32_t HWD_Timer_GetElapsed(uint32_t start, uint32_t end)
{
    // Правильна обробка переповнення
    return (end - start);
}
