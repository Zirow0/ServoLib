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

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Прапорці увімкнення складових PID регулятора
 */
typedef enum {
    PID_ENABLE_P = (1 << 0),  /**< Увімкнути пропорційну складову (0x01) */
    PID_ENABLE_I = (1 << 1),  /**< Увімкнути інтегральну складову (0x02) */
    PID_ENABLE_D = (1 << 2),  /**< Увімкнути диференціальну складову (0x04) */
    // Резерв для майбутніх розширень:
    // PID_ENABLE_FF = (1 << 3),  /**< Feed-forward складова */
    // PID_ENABLE_DF = (1 << 4),  /**< Derivative filter */
} PID_Enable_Flags_t;

/**
 * @brief Параметри PID регулятора
 */
typedef struct {
    float Kp;                /**< Пропорційний коефіцієнт */
    float Ki;                /**< Інтегральний коефіцієнт */
    float Kd;                /**< Диференціальний коефіцієнт */
    float out_min;           /**< Мінімальне значення виходу */
    float out_max;           /**< Максимальне значення виходу */
    uint8_t enabled_terms;   /**< Увімкнені складові (комбінація PID_Enable_Flags_t) */
} PID_Params_t;

/**
 * @brief Структура PID регулятора
 */
typedef struct {
    /* Параметри */
    PID_Params_t params;

    /* Внутрішній стан */
    float output;            /**< Вихід регулятора */
    float last_input;        /**< Попереднє значення входу (для D-term) */
    float integral;          /**< Накопичена інтегральна складова */

    /* Збережені складові (для debug) */
    float p_term;            /**< Остання пропорційна складова */
    float i_term;            /**< Остання інтегральна складова */
    float d_term;            /**< Остання диференціальна складова */

    /* Таймінг */
    uint32_t last_time_us;   /**< Час останнього виклику Compute (0 = перший виклик) */
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
 * @brief Обчислення виходу PID регулятора
 *
 * Основна функція, яка виконує PID обчислення на основі бажаного та поточного стану
 *
 * @param pid Вказівник на структуру PID
 * @param setpoint Бажане значення (уставка)
 * @param input Поточне значення процесної змінної
 * @param current_time_us Поточний час в мікросекундах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Compute(PID_Controller_t* pid, float setpoint, float input, uint32_t current_time_us);

/**
 * @brief Отримання виходу регулятора
 *
 * @param pid Вказівник на структуру PID
 * @return float Вихідне значення
 */
float PID_GetOutput(const PID_Controller_t* pid);

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
