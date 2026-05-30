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
#include "board_config.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

/* Private variables ---------------------------------------------------------*/

/* g_uptime_ms: при USE_FREERTOS завжди 0 — задовольняє extern у board_config.h,
 * але додатки мають використовувати HWD_Timer_GetMillis() / xTaskGetTickCount(). */
volatile uint32_t g_uptime_ms = 0;

/* SysTick interrupt handler -------------------------------------------------*/

#ifndef USE_FREERTOS
/* При USE_FREERTOS sys_tick_handler визначається FreeRTOS port.c
 * через макрос xPortSysTickHandler у FreeRTOSConfig.h. */
void sys_tick_handler(void)
{
    g_uptime_ms++;
}
#endif /* USE_FREERTOS */

/* Exported functions --------------------------------------------------------*/

uint32_t HWD_Timer_GetMillis(void)
{
#ifdef USE_FREERTOS
    /* xTaskGetTickCount() → ms (tick rate = 1 кГц, portTICK_PERIOD_MS = 1) */
    extern uint32_t xTaskGetTickCount(void);
    return xTaskGetTickCount();
#else
    return g_uptime_ms;
#endif
}

uint32_t HWD_Timer_GetMicros(void)
{
    /* TIM5 — 32-bit таймер, prescaler=99, clock=100MHz → 1 тік = 1 мкс */
    return timer_get_counter(MICROS_TIMER);
}

void HWD_Timer_DelayMs(uint32_t ms)
{
#ifdef USE_FREERTOS
    /* TIM5-based busy-wait: безпечно до vTaskStartScheduler() (init фаза).
     * Після старту планувальника викликається лише з init — допустимо. */
    uint32_t start = HWD_Timer_GetMicros();
    while ((HWD_Timer_GetMicros() - start) < (ms * 1000U)) {}
#else
    uint32_t start = g_uptime_ms;
    while ((g_uptime_ms - start) < ms) {}
#endif
}

void HWD_Timer_DelayUs(uint32_t us)
{
    uint32_t start = HWD_Timer_GetMicros();

    /* TIM5 — 32-bit, переповнення через ~71 хвилин → безпечна арифметика */
    while ((HWD_Timer_GetMicros() - start) < us) {
        /* busy-wait */
    }
}

