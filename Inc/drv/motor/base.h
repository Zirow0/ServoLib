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
 * @brief Базова структура даних двигуна
 */
typedef struct {
    Motor_Params_t params;           /**< Параметри двигуна */
    Motor_Stats_t stats;             /**< Статистика роботи */

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
 * @brief Зупинка двигуна
 *
 * Виконує логіку зупинки: скидання потужності, встановлення стану IDLE.
 * Драйвер повинен викликати hardware_stop_cb для зупинки апаратури.
 *
 * @param base Вказівник на базову структуру
 * @param hardware_stop_cb Callback для зупинки апаратної частини (PWM, GPIO)
 * @param driver_data Вказівник на дані драйвера (передається в callback)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_Stop(Motor_Base_Data_t* base,
                                Servo_Status_t (*hardware_stop_cb)(void*),
                                void* driver_data);

/**
 * @brief Аварійна зупинка двигуна
 *
 * Виконує аварійну зупинку: викликає hardware_stop_cb, встановлює emergency_flag,
 * переводить у стан ERROR.
 *
 * @param base Вказівник на базову структуру
 * @param hardware_stop_cb Callback для зупинки апаратної частини (PWM, GPIO)
 * @param driver_data Вказівник на дані драйвера (передається в callback)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_EmergencyStop(Motor_Base_Data_t* base,
                                         Servo_Status_t (*hardware_stop_cb)(void*),
                                         void* driver_data);

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
 * @brief Wrapper для зупинки через Motor_Interface_t
 *
 * Використовується драйверами для реєстрації в interface->stop.
 * Викликає Motor_Base_Stop() з даними з інтерфейсу.
 *
 * @param self Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_Stop_Wrapper(Motor_Interface_t* self);

/**
 * @brief Wrapper для аварійної зупинки через Motor_Interface_t
 *
 * Використовується драйверами для реєстрації в interface->emergency_stop.
 * Викликає Motor_Base_EmergencyStop() з даними з інтерфейсу.
 *
 * @param self Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Base_EmergencyStop_Wrapper(Motor_Interface_t* self);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_BASE_H */
