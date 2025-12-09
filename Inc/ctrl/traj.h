/**
 * @file traj.h
 * @brief Генератор траєкторій
 * @author ServoCore Team
 * @date 2025
 *
 * Модуль для плавного планування руху з обмеженням швидкості,
 * прискорення та ривка.
 */

#ifndef SERVOCORE_CTRL_TRAJ_H
#define SERVOCORE_CTRL_TRAJ_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Тип траєкторії
 */
typedef enum {
    TRAJ_TYPE_LINEAR  = 0,  /**< Лінійна траєкторія */
    TRAJ_TYPE_SCURVE  = 1,  /**< S-подібна траєкторія */
    TRAJ_TYPE_CUBIC   = 2,  /**< Кубічна траєкторія */
    TRAJ_TYPE_QUINTIC = 3   /**< Квінтична траєкторія */
} Trajectory_Type_t;

/**
 * @brief Стан траєкторії
 */
typedef enum {
    TRAJ_STATE_IDLE      = 0,  /**< Очікування */
    TRAJ_STATE_ACCEL     = 1,  /**< Прискорення */
    TRAJ_STATE_CRUISE    = 2,  /**< Постійна швидкість */
    TRAJ_STATE_DECEL     = 3,  /**< Уповільнення */
    TRAJ_STATE_COMPLETED = 4   /**< Завершено */
} Trajectory_State_t;

/**
 * @brief Параметри траєкторії
 */
typedef struct {
    Trajectory_Type_t type;  /**< Тип траєкторії */
    float max_velocity;      /**< Максимальна швидкість (град/с) */
    float max_acceleration;  /**< Максимальне прискорення (град/с²) */
    float max_jerk;          /**< Максимальний ривок (град/с³) */
} Trajectory_Params_t;

/**
 * @brief Генератор траєкторій
 */
typedef struct {
    Trajectory_Params_t params;  /**< Параметри */

    /* Поточний стан */
    Trajectory_State_t state;    /**< Стан траєкторії */
    float start_position;        /**< Початкове положення */
    float target_position;       /**< Цільове положення */
    float current_position;      /**< Поточне положення */
    float current_velocity;      /**< Поточна швидкість */
    float current_acceleration;  /**< Поточне прискорення */

    /* Часові параметри */
    uint32_t start_time_ms;      /**< Час старту */
    uint32_t total_time_ms;      /**< Загальний час руху */
    float elapsed_time;          /**< Минулий час (с) */

    bool is_active;              /**< Прапорець активності */
    bool is_initialized;         /**< Прапорець ініціалізації */
} Trajectory_Generator_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація генератора траєкторій
 *
 * @param traj Вказівник на генератор
 * @param params Параметри траєкторії
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Traj_Init(Trajectory_Generator_t* traj, const Trajectory_Params_t* params);

/**
 * @brief Старт нової траєкторії
 *
 * @param traj Вказівник на генератор
 * @param start Початкове положення
 * @param target Цільове положення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Traj_Start(Trajectory_Generator_t* traj, float start, float target);

/**
 * @brief Обчислення поточної точки траєкторії
 *
 * @param traj Вказівник на генератор
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Traj_Compute(Trajectory_Generator_t* traj);

/**
 * @brief Отримання поточного положення
 *
 * @param traj Вказівник на генератор
 * @return float Поточне положення
 */
float Traj_GetPosition(const Trajectory_Generator_t* traj);

/**
 * @brief Отримання поточної швидкості
 *
 * @param traj Вказівник на генератор
 * @return float Поточна швидкість
 */
float Traj_GetVelocity(const Trajectory_Generator_t* traj);

/**
 * @brief Перевірка чи траєкторія завершена
 *
 * @param traj Вказівник на генератор
 * @return bool true якщо завершена
 */
bool Traj_IsCompleted(const Trajectory_Generator_t* traj);

/**
 * @brief Зупинка траєкторії
 *
 * @param traj Вказівник на генератор
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Traj_Stop(Trajectory_Generator_t* traj);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_TRAJ_H */
