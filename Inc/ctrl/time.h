/**
 * @file time.h
 * @brief Керування таймінгами та частотою оновлення
 * @author ServoCore Team
 * @date 2025
 *
 * Модуль для забезпечення періодичності оновлення системи керування
 * та вимірювання продуктивності.
 */

#ifndef SERVOCORE_CTRL_TIME_H
#define SERVOCORE_CTRL_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура таймера періодичного виконання
 */
typedef struct {
    uint32_t period_ms;          /**< Період виконання (мс) */
    uint32_t last_time_ms;       /**< Час останнього виконання (мс) */
    uint32_t execution_count;    /**< Кількість виконань */
    bool is_running;             /**< Прапорець активності */
} Periodic_Timer_t;

/**
 * @brief Структура вимірювання часу виконання
 */
typedef struct {
    uint32_t start_time;         /**< Час початку (мкс) */
    uint32_t end_time;           /**< Час кінця (мкс) */
    uint32_t duration;           /**< Тривалість (мкс) */
    uint32_t min_duration;       /**< Мінімальна тривалість (мкс) */
    uint32_t max_duration;       /**< Максимальна тривалість (мкс) */
    uint32_t avg_duration;       /**< Середня тривалість (мкс) */
    uint32_t measurement_count;  /**< Кількість вимірювань */
} Execution_Timer_t;

/**
 * @brief Структура для Rate Limiter (обмежувач частоти)
 */
typedef struct {
    float max_rate;              /**< Максимальна швидкість зміни (одиниць/с) */
    float last_value;            /**< Попереднє значення */
    uint32_t last_time_ms;       /**< Час останнього оновлення (мс) */
    bool is_initialized;         /**< Прапорець ініціалізації */
} Rate_Limiter_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація періодичного таймера
 *
 * @param timer Вказівник на структуру таймера
 * @param period_ms Період виконання в мілісекундах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_InitPeriodicTimer(Periodic_Timer_t* timer, uint32_t period_ms);

/**
 * @brief Перевірка чи настав час виконання
 *
 * @param timer Вказівник на структуру таймера
 * @return bool true якщо час настав
 */
bool Time_IsElapsed(Periodic_Timer_t* timer);

/**
 * @brief Скидання таймера
 *
 * @param timer Вказівник на структуру таймера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_ResetTimer(Periodic_Timer_t* timer);

/**
 * @brief Отримання фактичної частоти виконання
 *
 * @param timer Вказівник на структуру таймера
 * @return float Частота в Гц
 */
float Time_GetActualFrequency(const Periodic_Timer_t* timer);

/**
 * @brief Ініціалізація вимірювача часу виконання
 *
 * @param timer Вказівник на структуру вимірювача
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_InitExecutionTimer(Execution_Timer_t* timer);

/**
 * @brief Початок вимірювання часу
 *
 * @param timer Вказівник на структуру вимірювача
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_StartMeasurement(Execution_Timer_t* timer);

/**
 * @brief Кінець вимірювання часу
 *
 * @param timer Вказівник на структуру вимірювача
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_StopMeasurement(Execution_Timer_t* timer);

/**
 * @brief Отримання середнього часу виконання
 *
 * @param timer Вказівник на структуру вимірювача
 * @return uint32_t Середній час в мікросекундах
 */
uint32_t Time_GetAverageDuration(const Execution_Timer_t* timer);

/**
 * @brief Скидання статистики вимірювань
 *
 * @param timer Вказівник на структуру вимірювача
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_ResetMeasurements(Execution_Timer_t* timer);

/**
 * @brief Ініціалізація обмежувача швидкості
 *
 * @param limiter Вказівник на структуру обмежувача
 * @param max_rate Максимальна швидкість зміни (одиниць/секунду)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Time_InitRateLimiter(Rate_Limiter_t* limiter, float max_rate);

/**
 * @brief Застосування обмеження швидкості
 *
 * @param limiter Вказівник на структуру обмежувача
 * @param target Цільове значення
 * @return float Обмежене значення
 */
float Time_ApplyRateLimit(Rate_Limiter_t* limiter, float target);

/**
 * @brief Затримка в мілісекундах
 *
 * @param ms Час затримки в мілісекундах
 */
void Time_DelayMs(uint32_t ms);

/**
 * @brief Затримка в мікросекундах
 *
 * @param us Час затримки в мікросекундах
 */
void Time_DelayUs(uint32_t us);

/**
 * @brief Отримання поточного часу в мілісекундах
 *
 * @return uint32_t Час в мілісекундах від старту системи
 */
uint32_t Time_GetMillis(void);

/**
 * @brief Отримання поточного часу в мікросекундах
 *
 * @return uint32_t Час в мікросекундах
 */
uint32_t Time_GetMicros(void);

/**
 * @brief Конвертація частоти в період
 *
 * @param frequency Частота в Гц
 * @return uint32_t Період в мілісекундах
 */
uint32_t Time_FreqToPeriod(float frequency);

/**
 * @brief Конвертація періоду в частоту
 *
 * @param period_ms Період в мілісекундах
 * @return float Частота в Гц
 */
float Time_PeriodToFreq(uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_TIME_H */
