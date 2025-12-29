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
 * @brief Команда керування двигуном (універсальна)
 */
typedef struct {
    Motor_Type_t type;    /**< Тип команди */

    union {
        /** DC мотор: одна потужність */
        struct {
            float power;  /**< Потужність -100.0 .. +100.0 */
        } dc;

        /** Кроковий мотор: дві фази */
        struct {
            float phase_a;  /**< Фаза A: -100.0 .. +100.0 */
            float phase_b;  /**< Фаза B: -100.0 .. +100.0 */
        } stepper;

        /** BLDC мотор: три фази */
        struct {
            float phase_a;  /**< Фаза A: -100.0 .. +100.0 */
            float phase_b;  /**< Фаза B: -100.0 .. +100.0 */
            float phase_c;  /**< Фаза C: -100.0 .. +100.0 */
        } bldc;
    } data;
} Motor_Command_t;

/**
 * @brief Параметри двигуна
 */
typedef struct {
    Motor_Type_t type;           /**< Тип двигуна */
    float max_power;             /**< Максимальна потужність (%) */
    float min_power;             /**< Мінімальна потужність (%) */
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
     * @brief Встановлення команди керування (універсальна)
     * @param self Вказівник на інтерфейс
     * @param cmd Команда керування з union
     * @return Servo_Status_t Статус виконання
     */
    Servo_Status_t (*set_command)(Motor_Interface_t* self, const Motor_Command_t* cmd);

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
 * @brief Встановлення команди керування (універсальна)
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param cmd Команда керування
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_SetCommand(Motor_Interface_t* motor, const Motor_Command_t* cmd);

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

/* Wrapper функції для зручності -------------------------------------------*/

/**
 * @brief Встановлення потужності DC мотора
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param power Потужність -100.0 .. +100.0
 * @return Servo_Status_t Статус виконання
 */
static inline Servo_Status_t Motor_SetPower_DC(Motor_Interface_t* motor, float power)
{
    Motor_Command_t cmd = {
        .type = MOTOR_TYPE_DC_PWM,
        .data.dc.power = power
    };
    return Motor_SetCommand(motor, &cmd);
}

/**
 * @brief Встановлення фаз крокового мотора
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param phase_a Фаза A: -100.0 .. +100.0
 * @param phase_b Фаза B: -100.0 .. +100.0
 * @return Servo_Status_t Статус виконання
 */
static inline Servo_Status_t Motor_SetPower_Stepper(Motor_Interface_t* motor,
                                                     float phase_a, float phase_b)
{
    Motor_Command_t cmd = {
        .type = MOTOR_TYPE_STEPPER,
        .data.stepper.phase_a = phase_a,
        .data.stepper.phase_b = phase_b
    };
    return Motor_SetCommand(motor, &cmd);
}

/**
 * @brief Встановлення фаз BLDC мотора
 *
 * @param motor Вказівник на інтерфейс двигуна
 * @param phase_a Фаза A: -100.0 .. +100.0
 * @param phase_b Фаза B: -100.0 .. +100.0
 * @param phase_c Фаза C: -100.0 .. +100.0
 * @return Servo_Status_t Статус виконання
 */
static inline Servo_Status_t Motor_SetPower_BLDC(Motor_Interface_t* motor,
                                                  float phase_a,
                                                  float phase_b,
                                                  float phase_c)
{
    Motor_Command_t cmd = {
        .type = MOTOR_TYPE_BLDC,
        .data.bldc.phase_a = phase_a,
        .data.bldc.phase_b = phase_b,
        .data.bldc.phase_c = phase_c
    };
    return Motor_SetCommand(motor, &cmd);
}

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_IFACE_MOTOR_H */
