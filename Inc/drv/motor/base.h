/**
 * @file base.h
 * @brief Базовий драйвер двигуна
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить базову реалізацію драйвера двигуна з загальною логікою,
 * яка використовується всіма типами двигунів.
 */

#ifndef SERVOCORE_DRV_MOTOR_BASE_H
#define SERVOCORE_DRV_MOTOR_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../iface/motor.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Режим захисту двигуна
 */
typedef enum {
    MOTOR_PROTECTION_NONE        = 0x00,  /**< Без захисту */
    MOTOR_PROTECTION_OVERCURRENT = 0x01,  /**< Захист від перевантаження */
    MOTOR_PROTECTION_OVERHEAT    = 0x02,  /**< Захист від перегріву */
    MOTOR_PROTECTION_STALL       = 0x04   /**< Захист від заклинювання */
} Motor_Protection_t;

/**
 * @brief Конфігурація захисту двигуна
 */
typedef struct {
    uint32_t max_current;        /**< Максимальний струм (mA) */
    uint32_t max_temperature;    /**< Максимальна температура (°C) */
    uint32_t stall_timeout_ms;   /**< Таймаут визначення заклинювання (мс) */
    uint8_t protection_mask;     /**< Маска активних захистів */
} Motor_Protection_Config_t;

/**
 * @brief Базова структура даних двигуна
 */
typedef struct {
    Motor_Params_t params;           /**< Параметри двигуна */
    Motor_Stats_t stats;             /**< Статистика роботи */
    Motor_Protection_Config_t prot;  /**< Конфігурація захисту */

    Motor_State_t state;             /**< Поточний стан */
    float current_power;             /**< Поточна потужність (%) */
    Motor_Direction_t direction;     /**< Напрямок обертання */

    uint32_t start_time_ms;          /**< Час останнього запуску */
    uint32_t last_update_ms;         /**< Час останнього оновлення */

    bool is_initialized;             /**< Прапорець ініціалізації */
    bool emergency_flag;             /**< Прапорець аварійного режиму */

    Servo_Error_t last_error;        /**< Код останньої помилки */
} Motor_Base_Data_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація базових даних двигуна
 *
 * @param base Вказівник на базову структуру
 * @param params Параметри двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_Init(Motor_Base_Data_t* base, const Motor_Params_t* params);

/**
 * @brief Деініціалізація базових даних
 *
 * @param base Вказівник на базову структуру
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_DeInit(Motor_Base_Data_t* base);

/**
 * @brief Оновлення стану двигуна
 *
 * @param base Вказівник на базову структуру
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_Update(Motor_Base_Data_t* base);

/**
 * @brief Встановлення потужності
 *
 * @param base Вказівник на базову структуру
 * @param power Потужність (-100.0 до +100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_SetPower(Motor_Base_Data_t* base, float power);

/**
 * @brief Перевірка захистів
 *
 * @param base Вказівник на базову структуру
 * @param current Поточний струм (mA)
 * @param temperature Поточна температура (°C)
 * @return Servo_Status_t SERVO_OK якщо все в нормі, SERVO_ERROR при спрацюванні захисту
 */
Servo_Status_t Motor_Base_CheckProtection(Motor_Base_Data_t* base,
                                          uint32_t current,
                                          uint32_t temperature);

/**
 * @brief Скидання помилки
 *
 * @param base Вказівник на базову структуру
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_ClearError(Motor_Base_Data_t* base);

/**
 * @brief Встановлення стану двигуна
 *
 * @param base Вказівник на базову структуру
 * @param state Новий стан
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_SetState(Motor_Base_Data_t* base, Motor_State_t state);

/**
 * @brief Отримання статистики
 *
 * @param base Вказівник на базову структуру
 * @param stats Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_GetStats(Motor_Base_Data_t* base, Motor_Stats_t* stats);

/**
 * @brief Налаштування параметрів захисту
 *
 * @param base Вказівник на базову структуру
 * @param prot Конфігурація захисту
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_ConfigProtection(Motor_Base_Data_t* base,
                                           const Motor_Protection_Config_t* prot);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_BASE_H */
