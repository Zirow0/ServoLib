/**
 * @file motor.h
 * @brief Інтерфейс та базова реалізація драйвера DC двигуна
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
 * @brief Параметри двигуна
 */
typedef struct {
    float max_power;          /**< Максимальна потужність (%) */
    float min_power;          /**< Мінімальна потужність — мертва зона (%) */
    float max_power_rate;     /**< Макс. зміна потужності за один виклик (%), 0 = без обмеження */
    bool  invert_direction;   /**< Інвертувати напрямок */
} Motor_Params_t;

/**
 * @brief Дані стану двигуна
 */
typedef struct {
    Motor_Params_t    params;           /**< Параметри двигуна */

    Motor_State_t     state;            /**< Поточний стан */
    Motor_Direction_t direction;        /**< Напрямок обертання */
    float             current_power;    /**< Поточна потужність (%) */
    float             prev_power;       /**< Потужність попереднього виклику (для rate limit) */

    bool              is_initialized;   /**< Прапорець ініціалізації */
    bool              emergency_flag;   /**< Прапорець аварійного режиму */
    Servo_Error_t     last_error;       /**< Код останньої помилки */
} Motor_Data_t;

/**
 * @brief Hardware callbacks для драйвера мотора
 */
typedef struct {
    /** @brief Ініціалізація апаратури (PWM каналів, GPIO) */
    Servo_Status_t (*init)(void* driver_data, const Motor_Params_t* params);

    /** @brief Встановлення потужності на апаратуру */
    Servo_Status_t (*set_power)(void* driver_data, float processed_power);

    /** @brief Зупинка апаратури (PWM = 0) */
    Servo_Status_t (*stop)(void* driver_data);
} Motor_Hardware_Callbacks_t;

/**
 * @brief Структура інтерфейсу двигуна
 */
typedef struct Motor_Interface Motor_Interface_t;

struct Motor_Interface {
    Motor_Data_t              data;        /**< Дані двигуна */
    Motor_Hardware_Callbacks_t hw;         /**< Hardware callbacks */
    void*                     driver_data; /**< Вказівник на конкретну реалізацію */
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація драйвера двигуна
 */
Servo_Status_t Motor_Init(Motor_Interface_t* motor, const Motor_Params_t* params);

/**
 * @brief Встановлення потужності DC двигуна (-100.0 .. +100.0)
 *
 * Застосовує обмеження max/min, мертву зону, rate limit та інверсію.
 */
Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, float power);

/**
 * @brief Зупинка двигуна (з rate limit обходом)
 */
Servo_Status_t Motor_Stop(Motor_Interface_t* motor);

/**
 * @brief Аварійна зупинка — миттєво, без rate limit
 */
Servo_Status_t Motor_EmergencyStop(Motor_Interface_t* motor);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_MOTOR_H */
