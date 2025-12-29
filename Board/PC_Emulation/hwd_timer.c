/**
 * @file hwd_timer.c
 * @brief Реалізація HWD Timer для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить емуляцію таймера для емуляції на ПК.
 */

/* Includes ------------------------------------------------------------------*/
#include "../hwd/hwd_timer.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>

/* Private defines -----------------------------------------------------------*/
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

/* Private variables ---------------------------------------------------------*/
static uint32_t start_time_ms = 0;

/* Private function prototypes -----------------------------------------------*/
static uint32_t GetSystemTimeMs(void);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_Timer_Init(void)
{
    start_time_ms = GetSystemTimeMs();
    return SERVO_OK;
}

uint32_t HWD_Timer_GetMillis(void)
{
    return GetSystemTimeMs() - start_time_ms;
}

uint32_t HWD_Timer_GetMicros(void)
{
    return (GetSystemTimeMs() - start_time_ms) * 1000;
}

void HWD_Timer_DelayMs(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000};
    nanosleep(&ts, NULL);
#endif
}

void HWD_Timer_DelayUs(uint32_t us)
{
    // Для емуляції використовуємо мілісекундну точність
    uint32_t ms = (us + 999) / 1000; // Округлення вгору
    HWD_Timer_DelayMs(ms);
}

/* Private functions ---------------------------------------------------------*/

static uint32_t GetSystemTimeMs(void)
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}