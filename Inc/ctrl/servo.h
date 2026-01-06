/**
 * @file servo.h
 * @brief Головний контролер сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Координація всіх підсистем сервоприводу
 */

#ifndef SERVOCORE_CTRL_SERVO_H
#define SERVOCORE_CTRL_SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../drv/motor/motor.h"
#include "../drv/brake/brake.h"
#include "pid.h"
#include "safety.h"
#include "err.h"
#include "traj.h"
#include "calib.h"
#include "time.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація сервоприводу
 */
typedef struct {
    /* Параметри осі */
    Axis_Config_t axis_config;

    /* Параметри PID */
    PID_Params_t pid_params;

    /* Параметри безпеки */
    Safety_Config_t safety_config;

    /* Параметри траєкторії */
    Trajectory_Params_t traj_params;

    /* Параметри гальм (застаріло - налаштовуються в GPIO_Brake_Create) */
    // Brake_Params_t brake_params;  // Тепер не потрібно, таймінги задаються при створенні GPIO драйвера

    /* Частота оновлення (Hz) */
    float update_frequency;

    /* Прапорці */
    bool enable_brake;              /**< Увімкнути використання гальм */
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
    Motor_Interface_t* motor;       /**< Інтерфейс двигуна */
    PID_Controller_t pid;           /**< PID регулятор */
    Safety_System_t safety;         /**< Система безпеки */
    Error_Manager_t error_mgr;      /**< Менеджер помилок */
    Trajectory_Generator_t traj;    /**< Генератор траєкторій */
    Calibration_Data_t calib;       /**< Дані калібрування */
    Brake_Interface_t* brake;       /**< Інтерфейс гальм (опціонально) */

    /* Таймінги */
    Periodic_Timer_t update_timer;  /**< Таймер оновлення */

    /* Прапорці */
    bool is_initialized;            /**< Прапорець ініціалізації */
    bool enable_trajectory;         /**< Увімкнути генератор траєкторій */
} Servo_Controller_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація сервоприводу
 *
 * @param servo Вказівник на контролер
 * @param config Конфігурація
 * @param motor Інтерфейс двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_Init(Servo_Controller_t* servo,
                          const Servo_Config_t* config,
                          Motor_Interface_t* motor);

/**
 * @brief Ініціалізація сервоприводу з гальмами
 *
 * @param servo Вказівник на контролер
 * @param config Конфігурація
 * @param motor Інтерфейс двигуна
 * @param brake Інтерфейс гальм (NULL якщо не використовуються)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_InitWithBrake(Servo_Controller_t* servo,
                                    const Servo_Config_t* config,
                                    Motor_Interface_t* motor,
                                    Brake_Interface_t* brake);

/**
 * @brief Основний цикл оновлення сервоприводу
 *
 * Викликати періодично в головному циклі
 *
 * @param servo Вказівник на контролер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_Update(Servo_Controller_t* servo);

/**
 * @brief Встановлення цільового положення
 *
 * @param servo Вказівник на контролер
 * @param position Цільове положення (градуси)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position);

/**
 * @brief Встановлення швидкості
 *
 * @param servo Вказівник на контролер
 * @param velocity Швидкість (град/с)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity);

/**
 * @brief Зупинка сервоприводу
 *
 * @param servo Вказівник на контролер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_Stop(Servo_Controller_t* servo);

/**
 * @brief Аварійна зупинка
 *
 * @param servo Вказівник на контролер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo);

/**
 * @brief Отримання поточного положення
 *
 * @param servo Вказівник на контролер
 * @return float Поточне положення (градуси)
 */
float Servo_GetPosition(const Servo_Controller_t* servo);

/**
 * @brief Отримання поточної швидкості
 *
 * @param servo Вказівник на контролер
 * @return float Поточна швидкість (град/с)
 */
float Servo_GetVelocity(const Servo_Controller_t* servo);

/**
 * @brief Отримання стану
 *
 * @param servo Вказівник на контролер
 * @return Servo_State_t Поточний стан
 */
Servo_State_t Servo_GetState(const Servo_Controller_t* servo);

/**
 * @brief Калібрування нульового положення
 *
 * @param servo Вказівник на контролер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_CalibrateZero(Servo_Controller_t* servo);

/**
 * @brief Увімкнення/вимкнення режиму траєкторій
 *
 * @param servo Вказівник на контролер
 * @param enable true для увімкнення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Servo_EnableTrajectory(Servo_Controller_t* servo, bool enable);

/**
 * @brief Перевірка чи досягнуто цільового положення
 *
 * @param servo Вказівник на контролер
 * @return bool true якщо досягнуто
 */
bool Servo_IsAtTarget(const Servo_Controller_t* servo);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_SERVO_H */
