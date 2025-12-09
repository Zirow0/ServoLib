/**
 * @file motor.h
 * @brief Абстрактний інтерфейс керування двигуном
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл визначає незалежний від апаратного забезпечення інтерфейс
 * для керування будь-яким типом двигуна (DC, BLDC, stepper, тощо).
 */

#ifndef SERVOCORE_IFACE_MOTOR_H
#define SERVOCORE_IFACE_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Параметри двигуна
 */
typedef struct {
    Motor_Type_t type;           /**< Тип двигуна */
    float max_power;             /**< Максимальна потужність (%) */
    float min_power;             /**< Мінімальна потужність (%) */
    float max_current;           /**< Максимальний струм (A) */
    float max_temperature;       /**< Максимальна температура (°C) */
    uint32_t max_rpm;            /**< Максимальна швидкість (об/хв) */
    bool invert_direction;       /**< Інвертувати напрямок */
} Motor_Params_t;

/**
 * @brief Статистика двигуна
 */
typedef struct {
    uint32_t run_time_ms;        /**< Час роботи (мс) */
    uint32_t total_starts;       /**< Загальна кількість запусків */
    uint32_t error_count;        /**< Кількість помилок */
    float current_power;         /**< Поточна потужність (%) */
    uint32_t current_draw;       /**< Поточний струм (mA) */
    Motor_State_t state;         /**< Поточний стан */
    Motor_Direction_t direction; /**< Напрямок обертання */
} Motor_Stats_t;

/**
 * @brief Структура інтерфейсу двигуна
 *
 * Ця структура визначає набір функцій-вказівників для роботи з двигуном.
 * Кожна конкретна реалізація драйвера повинна надати ці функції.
 */
typedef struct Motor_Interface Motor_Interface_t;

struct Motor_Interface {
    /**
     * @brief Ініціалізація двигуна
     * @param self Вказівник на інтерфейс
     * @param params Параметри двигуна
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*init)(Motor_Interface_t* self, const Motor_Params_t* params);

    /**
     * @brief Деініціалізація двигуна
     * @param self Вказівник на інтерфейс
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*deinit)(Motor_Interface_t* self);

    /**
     * @brief Встановлення потужності двигуна
     * @param self Вказівник на інтерфейс
     * @param power Потужність (-100.0 до +100.0), знак визначає напрямок
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*set_power)(Motor_Interface_t* self, float power);

    /**
     * @brief Зупинка двигуна
     * @param self Вказівник на інтерфейс
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*stop)(Motor_Interface_t* self);

    /**
     * @brief Аварійна зупинка
     * @param self Вказівник на інтерфейс
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*emergency_stop)(Motor_Interface_t* self);

    /**
     * @brief Встановлення напрямку обертання
     * @param self Вказівник на інтерфейс
     * @param direction Напрямок обертання
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*set_direction)(Motor_Interface_t* self, Motor_Direction_t direction);

    /**
     * @brief Отримання стану двигуна
     * @param self Вказівник на інтерфейс
     * @param state Вказівник для збереження стану
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*get_state)(Motor_Interface_t* self, Motor_State_t* state);

    /**
     * @brief Отримання статистики
     * @param self Вказівник на інтерфейс
     * @param stats Вказівник для збереження статистики
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*get_stats)(Motor_Interface_t* self, Motor_Stats_t* stats);

    /**
     * @brief Оновлення стану двигуна (викликається періодично)
     * @param self Вказівник на інтерфейс
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*update)(Motor_Interface_t* self);

    /** @brief Вказівник на конкретну реалізацію драйвера */
    void* driver_data;
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param params Параметри двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Init(Motor_Interface_t* motor, const Motor_Params_t* params);

/**
 * @brief Деініціалізація двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_DeInit(Motor_Interface_t* motor);

/**
 * @brief Встановлення потужності двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param power Потужність (-100.0 до +100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, float power);

/**
 * @brief Зупинка двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Stop(Motor_Interface_t* motor);

/**
 * @brief Аварійна зупинка двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_EmergencyStop(Motor_Interface_t* motor);

/**
 * @brief Встановлення напрямку обертання
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param direction Напрямок обертання
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_SetDirection(Motor_Interface_t* motor, Motor_Direction_t direction);

/**
 * @brief Отримання стану двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param state Вказівник для збереження стану
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_GetState(Motor_Interface_t* motor, Motor_State_t* state);

/**
 * @brief Отримання статистики двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param stats Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_GetStats(Motor_Interface_t* motor, Motor_Stats_t* stats);

/**
 * @brief Оновлення стану двигуна
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Update(Motor_Interface_t* motor);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_IFACE_MOTOR_H */
