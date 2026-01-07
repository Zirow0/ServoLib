/**
 * @file pid.h
 * @brief PID регулятор
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізація пропорційно-інтегрально-диференціального (PID) регулятора
 * з anti-windup механізмом та обмеженням виходу.
 */

#ifndef SERVOCORE_CTRL_PID_H
#define SERVOCORE_CTRL_PID_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Режим роботи PID регулятора
 */
typedef enum {
    PID_MODE_MANUAL    = 0,  /**< Ручний режим (PID не активний) */
    PID_MODE_AUTOMATIC = 1   /**< Автоматичний режим */
} PID_Mode_t;

/**
 * @brief Напрямок дії регулятора
 */
typedef enum {
    PID_DIRECTION_DIRECT  = 0,  /**< Прямий (збільшення виходу при збільшенні помилки) */
    PID_DIRECTION_REVERSE = 1   /**< Зворотний (зменшення виходу при збільшенні помилки) */
} PID_Direction_t;

/**
 * @brief Параметри PID регулятора
 */
typedef struct {
    float Kp;                /**< Пропорційний коефіцієнт */
    float Ki;                /**< Інтегральний коефіцієнт */
    float Kd;                /**< Диференціальний коефіцієнт */
    float sample_time;       /**< Час вибірки (секунди) */
    float out_min;           /**< Мінімальне значення виходу */
    float out_max;           /**< Максимальне значення виходу */
    PID_Direction_t direction; /**< Напрямок дії */
} PID_Params_t;

/**
 * @brief Структура PID регулятора
 */
typedef struct {
    /* Параметри */
    PID_Params_t params;

    /* Внутрішній стан */
    float setpoint;          /**< Задане значення */
    float input;             /**< Поточне значення входу */
    float output;            /**< Вихід регулятора */

    float last_input;        /**< Попереднє значення входу */
    float integral;          /**< Накопичена інтегральна складова */

    /* Налаштування */
    PID_Mode_t mode;         /**< Режим роботи */
    bool is_initialized;     /**< Прапорець ініціалізації */

    /* Статистика */
    uint32_t compute_count;  /**< Кількість обчислень */
    float last_compute_time; /**< Час останнього обчислення */
} PID_Controller_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація PID регулятора
 *
 * @param pid Вказівник на структуру PID
 * @param params Параметри регулятора
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Init(PID_Controller_t* pid, const PID_Params_t* params);

/**
 * @brief Встановлення коефіцієнтів PID
 *
 * @param pid Вказівник на структуру PID
 * @param Kp Пропорційний коефіцієнт
 * @param Ki Інтегральний коефіцієнт
 * @param Kd Диференціальний коефіцієнт
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetTunings(PID_Controller_t* pid, float Kp, float Ki, float Kd);

/**
 * @brief Встановлення меж виходу
 *
 * @param pid Вказівник на структуру PID
 * @param min Мінімальне значення
 * @param max Максимальне значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetOutputLimits(PID_Controller_t* pid, float min, float max);

/**
 * @brief Встановлення режиму роботи
 *
 * @param pid Вказівник на структуру PID
 * @param mode Режим (MANUAL або AUTOMATIC)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetMode(PID_Controller_t* pid, PID_Mode_t mode);

/**
 * @brief Встановлення напрямку дії
 *
 * @param pid Вказівник на структуру PID
 * @param direction Напрямок (DIRECT або REVERSE)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetDirection(PID_Controller_t* pid, PID_Direction_t direction);

/**
 * @brief Встановлення заданого значення
 *
 * @param pid Вказівник на структуру PID
 * @param setpoint Задане значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetSetpoint(PID_Controller_t* pid, float setpoint);

/**
 * @brief Встановлення часу вибірки
 *
 * @param pid Вказівник на структуру PID
 * @param sample_time Час вибірки (секунди)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_SetSampleTime(PID_Controller_t* pid, float sample_time);

/**
 * @brief Обчислення виходу PID регулятора
 *
 * Основна функція, яка виконує PID обчислення на основі поточного входу
 *
 * @param pid Вказівник на структуру PID
 * @param input Поточне значення процесної змінної
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Compute(PID_Controller_t* pid, float input);

/**
 * @brief Отримання виходу регулятора
 *
 * @param pid Вказівник на структуру PID
 * @return float Вихідне значення
 */
float PID_GetOutput(const PID_Controller_t* pid);

/**
 * @brief Отримання поточної помилки
 *
 * @param pid Вказівник на структуру PID
 * @return float Поточна помилка (setpoint - input)
 */
float PID_GetError(const PID_Controller_t* pid);

/**
 * @brief Скидання регулятора
 *
 * Очищає інтегральну та диференціальну складові
 *
 * @param pid Вказівник на структуру PID
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Reset(PID_Controller_t* pid);

/**
 * @brief Отримання пропорційної складової
 *
 * @param pid Вказівник на структуру PID
 * @return float P складова
 */
float PID_GetPTerm(const PID_Controller_t* pid);

/**
 * @brief Отримання інтегральної складової
 *
 * @param pid Вказівник на структуру PID
 * @return float I складова
 */
float PID_GetITerm(const PID_Controller_t* pid);

/**
 * @brief Отримання диференціальної складової
 *
 * @param pid Вказівник на структуру PID
 * @return float D складова
 */
float PID_GetDTerm(const PID_Controller_t* pid);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_PID_H */
