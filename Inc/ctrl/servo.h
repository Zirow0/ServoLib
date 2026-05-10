/**
 * @file servo.h
 * @brief Головний контролер сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Координація всіх підсистем сервоприводу.
 * Одиниці вимірювання: кут — рад, швидкість — рад/с.
 */

#ifndef SERVOCORE_CTRL_SERVO_H
#define SERVOCORE_CTRL_SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../config.h"
#include "../drv/motor/motor.h"
#include "../drv/position/position.h"
#include "../drv/brake/brake.h"
#include "../drv/current/current.h"
#include "cascade.h"
#include "safety.h"
#include "traj.h"
#include "time.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація сервоприводу
 */
typedef struct {
    /* Параметри осі */
    Axis_Config_t axis_config;

    /* Каскадний PID */
    Cascade_Config_t cascade_config;

    /* Параметри безпеки */
    Safety_Config_t safety_config;

    /* Параметри траєкторії */
    Trajectory_Params_t traj_params;

    /* Частота оновлення (Hz) */
    float update_frequency;

    /* Прапорці */
    bool enable_brake;  /**< Увімкнути використання гальм */
} Servo_Config_t;

/**
 * @brief Головна структура сервоприводу
 */
typedef struct {
    /* Конфігурація */
    Servo_Config_t config;

    /* Стан осі */
    Axis_State_t state;

    /* Компоненти */
    Motor_Interface_t*          motor;    /**< Інтерфейс двигуна */
    Position_Sensor_Interface_t* sensor;  /**< Інтерфейс датчика положення (опціонально) */
    Brake_Interface_t*          brake;    /**< Інтерфейс гальм (опціонально) */
    Current_Sensor_Interface_t* current;  /**< Інтерфейс датчика струму (опціонально) */

    /* Регулятори */
    Cascade_Controller_t cascade;  /**< Каскадний PID регулятор */
    Safety_System_t      safety;   /**< Система безпеки */
    Trajectory_Generator_t traj;   /**< Генератор траєкторій */

    /* Таймінги */
    Periodic_Timer_t update_timer;  /**< Таймер оновлення */

    /* Прапорці */
    bool is_initialized;     /**< Прапорець ініціалізації */
    bool enable_trajectory;  /**< Увімкнути генератор траєкторій */
} Servo_Controller_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація сервоприводу (лише мотор)
 */
Servo_Status_t Servo_Init(Servo_Controller_t* servo,
                          const Servo_Config_t* config,
                          Motor_Interface_t* motor);

/**
 * @brief Повна ініціалізація сервоприводу з усіма компонентами
 *
 * @param servo   Вказівник на контролер
 * @param config  Конфігурація
 * @param motor   Інтерфейс двигуна
 * @param sensor  Датчик положення (NULL якщо не використовується)
 * @param brake   Гальмо (NULL якщо не використовується)
 * @param current Датчик струму (NULL якщо не використовується)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_InitFull(Servo_Controller_t* servo,
                               const Servo_Config_t* config,
                               Motor_Interface_t* motor,
                               Position_Sensor_Interface_t* sensor,
                               Brake_Interface_t* brake,
                               Current_Sensor_Interface_t* current);

/**
 * @brief Основний цикл оновлення — викликати періодично
 */
Servo_Status_t Servo_Update(Servo_Controller_t* servo);

/**
 * @brief Встановлення цільового положення (рад)
 */
Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position_rad);

/**
 * @brief Встановлення цільової кутової швидкості (рад/с)
 */
Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity_rad_s);

/**
 * @brief Встановлення цільового струму (А)
 */
Servo_Status_t Servo_SetTorque(Servo_Controller_t* servo, float current_a);

/**
 * @brief Зупинка сервоприводу
 */
Servo_Status_t Servo_Stop(Servo_Controller_t* servo);

/**
 * @brief Аварійна зупинка
 */
Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo);

/**
 * @brief Поточне положення (рад)
 */
float Servo_GetPosition(const Servo_Controller_t* servo);

/**
 * @brief Поточна кутова швидкість (рад/с)
 */
float Servo_GetVelocity(const Servo_Controller_t* servo);

/**
 * @brief Поточний стан
 */
Servo_State_t Servo_GetState(const Servo_Controller_t* servo);

/**
 * @brief Калібрування нульового положення
 */
Servo_Status_t Servo_CalibrateZero(Servo_Controller_t* servo);

/**
 * @brief Увімкнення/вимкнення режиму траєкторій
 */
Servo_Status_t Servo_EnableTrajectory(Servo_Controller_t* servo, bool enable);

/**
 * @brief Перевірка чи досягнуто цільового положення
 */
bool Servo_IsAtTarget(const Servo_Controller_t* servo);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_SERVO_H */
