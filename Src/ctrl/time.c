/**
 * @file time.c
 * @brief Реалізація керування часом
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/time.h"
#include "../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Time_InitPeriodicTimer(Periodic_Timer_t* timer, uint32_t period_ms)
{
    if (timer == NULL || period_ms == 0) {
        return SERVO_INVALID;
    }

    memset(timer, 0, sizeof(Periodic_Timer_t));

    timer->period_ms = period_ms;
    timer->last_time_ms = HWD_Timer_GetMillis();
    timer->is_running = true;

    return SERVO_OK;
}

bool Time_IsElapsed(Periodic_Timer_t* timer)
{
    if (timer == NULL || !timer->is_running) {
        return false;
    }

    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t elapsed = current_time - timer->last_time_ms;

    if (elapsed >= timer->period_ms) {
        timer->last_time_ms = current_time;
        timer->execution_count++;
        return true;
    }

    return false;
}

Servo_Status_t Time_ResetTimer(Periodic_Timer_t* timer)
{
    if (timer == NULL) {
        return SERVO_INVALID;
    }

    timer->last_time_ms = HWD_Timer_GetMillis();
    timer->execution_count = 0;

    return SERVO_OK;
}

float Time_GetActualFrequency(const Periodic_Timer_t* timer)
{
    if (timer == NULL || timer->execution_count == 0) {
        return 0.0f;
    }

    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t total_time = current_time - timer->last_time_ms +
                          (timer->execution_count * timer->period_ms);

    if (total_time == 0) {
        return 0.0f;
    }

    return (float)timer->execution_count * 1000.0f / (float)total_time;
}

Servo_Status_t Time_InitExecutionTimer(Execution_Timer_t* timer)
{
    if (timer == NULL) {
        return SERVO_INVALID;
    }

    memset(timer, 0, sizeof(Execution_Timer_t));
    timer->min_duration = 0xFFFFFFFF;  // Максимальне значення

    return SERVO_OK;
}

Servo_Status_t Time_StartMeasurement(Execution_Timer_t* timer)
{
    if (timer == NULL) {
        return SERVO_INVALID;
    }

    timer->start_time = Time_GetMicros();

    return SERVO_OK;
}

Servo_Status_t Time_StopMeasurement(Execution_Timer_t* timer)
{
    if (timer == NULL) {
        return SERVO_INVALID;
    }

    timer->end_time = Time_GetMicros();
    timer->duration = timer->end_time - timer->start_time;

    // Оновлення статистики
    if (timer->duration < timer->min_duration) {
        timer->min_duration = timer->duration;
    }

    if (timer->duration > timer->max_duration) {
        timer->max_duration = timer->duration;
    }

    // Розрахунок ковзного середнього
    timer->measurement_count++;
    if (timer->measurement_count == 1) {
        timer->avg_duration = timer->duration;
    } else {
        // Експоненціальне ковзне середнє з альфа = 0.1
        timer->avg_duration = (timer->avg_duration * 9 + timer->duration) / 10;
    }

    return SERVO_OK;
}

uint32_t Time_GetAverageDuration(const Execution_Timer_t* timer)
{
    if (timer == NULL) {
        return 0;
    }

    return timer->avg_duration;
}

Servo_Status_t Time_ResetMeasurements(Execution_Timer_t* timer)
{
    if (timer == NULL) {
        return SERVO_INVALID;
    }

    timer->min_duration = 0xFFFFFFFF;
    timer->max_duration = 0;
    timer->avg_duration = 0;
    timer->measurement_count = 0;

    return SERVO_OK;
}

void Time_DelayMs(uint32_t ms)
{
    HWD_Timer_DelayMs(ms);
}

void Time_DelayUs(uint32_t us)
{
    HWD_Timer_DelayUs(us);
}

uint32_t Time_GetMillis(void)
{
    return HWD_Timer_GetMillis();
}

uint32_t Time_GetMicros(void)
{
    return HWD_Timer_GetMicros();
}

uint32_t Time_FreqToPeriod(float frequency)
{
    if (frequency <= 0.0f) {
        return 0;
    }

    return (uint32_t)(1000.0f / frequency);
}

float Time_PeriodToFreq(uint32_t period_ms)
{
    if (period_ms == 0) {
        return 0.0f;
    }

    return 1000.0f / (float)period_ms;
}
