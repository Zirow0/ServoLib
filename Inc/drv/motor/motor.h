/**
 * @file motor.h
 * @brief Інтерфейс та базова реалізація драйвера двигуна
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить інтерфейс Motor_Interface_t та базову реалізацію
 * з загальною логікою для всіх типів двигунів (DC, BLDC, Stepper).
 */

#ifndef SERVOCORE_DRV_MOTOR_MOTOR_H
#define SERVOCORE_DRV_MOTOR_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Команда керування двигуном (універсальна)
 */
struct Motor_Command {
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
};

typedef struct Motor_Command Motor_Command_t;

/**
 * @brief Параметри двигуна
 */
struct Motor_Params {
    Motor_Type_t type;           /**< Тип двигуна */
    float max_power;             /**< Максимальна потужність (%) */
    float min_power;             /**< Мінімальна потужність (%) */
    bool invert_direction;       /**< Інвертувати напрямок */
};

typedef struct Motor_Params Motor_Params_t;

/**
 * @brief Статистика двигуна
 */
struct Motor_Stats {
    uint32_t run_time_ms;        /**< Час роботи (мс) */
    uint32_t total_starts;       /**< Загальна кількість запусків */
    uint32_t error_count;        /**< Кількість помилок */
    float current_power;         /**< Поточна потужність (%) */
    Motor_State_t state;         /**< Поточний стан */
    Motor_Direction_t direction; /**< Напрямок обертання */
};

typedef struct Motor_Stats Motor_Stats_t;

/**
 * @brief Hardware callbacks для драйвера мотора
 *
 * Конкретний драйвер (PWM, BLDC, Stepper) надає ці функції
 * для керування апаратною частиною.
 */
typedef struct {
    /** @brief Ініціалізація апаратури (PWM каналів, GPIO) */
    Servo_Status_t (*init)(void* driver_data, const Motor_Params_t* params);

    /** @brief Деініціалізація апаратури */
    Servo_Status_t (*deinit)(void* driver_data);

    /** @brief Встановлення потужності на апаратуру (PWM duty cycle) */
    Servo_Status_t (*set_power)(void* driver_data, const Motor_Command_t* cmd, float processed_power);

    /** @brief Зупинка апаратури (PWM = 0) */
    Servo_Status_t (*stop)(void* driver_data);

    /** @brief Оновлення апаратури (якщо потрібно) */
    Servo_Status_t (*update)(void* driver_data);
} Motor_Hardware_Callbacks_t;

/**
 * @brief Структура даних двигуна
 */
typedef struct Motor_Data Motor_Data_t;

struct Motor_Data {
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
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація базового драйвера двигуна
 *
 * Викликає hardware init callback та ініціалізує базові дані.
 *
 * @param motor Вказівник на Motor_Interface_t
 * @param params Параметри двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Init(Motor_Interface_t* motor, const Motor_Params_t* params);

/**
 * @brief Деініціалізація базового драйвера
 *
 * @param motor Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_DeInit(Motor_Interface_t* motor);

/**
 * @brief Встановлення потужності двигуна
 *
 * Обробляє команду (обмеження, інверсія), викликає hardware callback.
 *
 * @param motor Вказівник на Motor_Interface_t
 * @param cmd Команда керування
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, const Motor_Command_t* cmd);

/**
 * @brief Зупинка двигуна
 *
 * Викликає hardware stop callback, встановлює стан IDLE.
 *
 * @param motor Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Stop(Motor_Interface_t* motor);

/**
 * @brief Аварійна зупинка двигуна
 *
 * Викликає hardware stop callback, встановлює emergency_flag та стан ERROR.
 *
 * @param motor Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_EmergencyStop(Motor_Interface_t* motor);

/**
 * @brief Отримання стану двигуна
 *
 * @param motor Вказівник на Motor_Interface_t
 * @param state Вказівник для збереження стану
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_GetState(Motor_Interface_t* motor, Motor_State_t* state);

/**
 * @brief Отримання статистики двигуна
 *
 * @param motor Вказівник на Motor_Interface_t
 * @param stats Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_GetStats(Motor_Interface_t* motor, Motor_Stats_t* stats);

/**
 * @brief Оновлення стану двигуна
 *
 * Викликає hardware update callback, оновлює статистику.
 *
 * @param motor Вказівник на Motor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Motor_Update(Motor_Interface_t* motor);

/**
 * @brief Структура інтерфейсу двигуна
 *
 * Містить Motor_Data_t для загальної логіки та hardware callbacks
 * для апаратної специфіки. Драйвер надає тільки hardware callbacks.
 */
typedef struct Motor_Interface Motor_Interface_t;

struct Motor_Interface {
    /** @brief Дані двигуна (логіка, стан, статистика) */
    Motor_Data_t data;

    /** @brief Hardware callbacks від конкретного драйвера */
    Motor_Hardware_Callbacks_t hw;

    /** @brief Вказівник на конкретну реалізацію драйвера */
    void* driver_data;
};

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
    return Motor_SetPower(motor, &cmd);
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
    return Motor_SetPower(motor, &cmd);
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
    return Motor_SetPower(motor, &cmd);
}

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_MOTOR_H */
