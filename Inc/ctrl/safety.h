/**
 * @file safety.h
 * @brief Система безпеки сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Модуль для забезпечення безпечної роботи через обмеження
 * положення, швидкості, струму та інших параметрів.
 */

#ifndef SERVOCORE_CTRL_SAFETY_H
#define SERVOCORE_CTRL_SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Тип обмеження
 */
typedef enum {
    SAFETY_LIMIT_NONE     = 0x00,
    SAFETY_LIMIT_POSITION = 0x01,
    SAFETY_LIMIT_VELOCITY = 0x02,
    SAFETY_LIMIT_CURRENT  = 0x04,
    SAFETY_LIMIT_ALL      = 0xFF
} Safety_LimitType_t;

/**
 * @brief Конфігурація меж безпеки
 */
typedef struct {
    /* Межі положення */
    float min_position;          /**< Мінімальне положення (град) */
    float max_position;          /**< Максимальне положення (град) */
    bool enable_position_limits; /**< Увімкнути обмеження положення */

    /* Межі швидкості */
    float max_velocity;          /**< Максимальна швидкість (град/с) */
    bool enable_velocity_limit;  /**< Увімкнути обмеження швидкості */

    /* Межі прискорення */
    float max_acceleration;      /**< Максимальне прискорення (град/с²) */
    bool enable_acceleration_limit; /**< Увімкнути обмеження прискорення */

    /* Струмовий захист */
    uint32_t max_current;        /**< Максимальний струм (mA) */
    uint32_t current_timeout_ms; /**< Таймаут перевантаження (мс) */
    bool enable_current_protection; /**< Увімкнути струмовий захист */

    /* Watchdog */
    uint32_t watchdog_timeout_ms; /**< Таймаут watchdog (мс) */
    bool enable_watchdog;         /**< Увімкнути watchdog */

    /* Температурний захист */
    uint32_t max_temperature;     /**< Максимальна температура (°C) */
    bool enable_thermal_protection; /**< Увімкнути температурний захист */
} Safety_Config_t;

/**
 * @brief Стан системи безпеки
 */
typedef struct {
    /* Поточні значення */
    float current_position;       /**< Поточне положення */
    float current_velocity;       /**< Поточна швидкість */
    uint32_t current_draw;        /**< Поточний струм */
    uint32_t current_temperature; /**< Поточна температура */

    /* Прапорці порушень */
    bool position_violated;       /**< Порушення межі положення */
    bool velocity_violated;       /**< Порушення межі швидкості */
    bool current_violated;        /**< Порушення межі струму */
    bool watchdog_violated;       /**< Порушення watchdog */
    bool thermal_violated;        /**< Порушення температури */

    /* Таймінги */
    uint32_t last_update_time;    /**< Час останнього оновлення */
    uint32_t overcurrent_start;   /**< Початок перевантаження */

    /* Стан */
    bool is_safe;                 /**< Загальний стан безпеки */
    Servo_Error_t last_violation; /**< Останнє порушення */
} Safety_State_t;

/**
 * @brief Система безпеки
 */
typedef struct {
    Safety_Config_t config;       /**< Конфігурація */
    Safety_State_t state;         /**< Стан */
    bool is_initialized;          /**< Прапорець ініціалізації */
} Safety_System_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація системи безпеки
 *
 * @param safety Вказівник на систему безпеки
 * @param config Конфігурація меж
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Safety_Init(Safety_System_t* safety,
                           const Safety_Config_t* config);

/**
 * @brief Оновлення стану системи безпеки
 *
 * @param safety Вказівник на систему безпеки
 * @param position Поточне положення (град)
 * @param velocity Поточна швидкість (град/с)
 * @param current Поточний струм (mA)
 * @param temperature Поточна температура (°C)
 * @return Servo_Status_t SERVO_OK якщо безпечно, SERVO_ERROR при порушенні
 */
Servo_Status_t Safety_Update(Safety_System_t* safety,
                             float position,
                             float velocity,
                             uint32_t current,
                             uint32_t temperature);

/**
 * @brief Перевірка положення
 *
 * @param safety Вказівник на систему безпеки
 * @param position Положення для перевірки
 * @return bool true якщо в межах
 */
bool Safety_CheckPosition(const Safety_System_t* safety, float position);

/**
 * @brief Перевірка швидкості
 *
 * @param safety Вказівник на систему безпеки
 * @param velocity Швидкість для перевірки
 * @return bool true якщо в межах
 */
bool Safety_CheckVelocity(const Safety_System_t* safety, float velocity);

/**
 * @brief Обмеження положення в безпечних межах
 *
 * @param safety Вказівник на систему безпеки
 * @param position Положення для обмеження
 * @return float Обмежене положення
 */
float Safety_ClampPosition(const Safety_System_t* safety, float position);

/**
 * @brief Обмеження швидкості в безпечних межах
 *
 * @param safety Вказівник на систему безпеки
 * @param velocity Швидкість для обмеження
 * @return float Обмежена швидкість
 */
float Safety_ClampVelocity(const Safety_System_t* safety, float velocity);

/**
 * @brief Скидання порушень
 *
 * @param safety Вказівник на систему безпеки
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Safety_ClearViolations(Safety_System_t* safety);

/**
 * @brief Перевірка чи система в безпечному стані
 *
 * @param safety Вказівник на систему безпеки
 * @return bool true якщо безпечно
 */
bool Safety_IsSafe(const Safety_System_t* safety);

/**
 * @brief Watchdog kick (скидання таймера watchdog)
 *
 * @param safety Вказівник на систему безпеки
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Safety_WatchdogKick(Safety_System_t* safety);

/**
 * @brief Увімкнення/вимкнення окремих захистів
 *
 * @param safety Вказівник на систему безпеки
 * @param limit_type Тип обмеження
 * @param enable true для увімкнення, false для вимкнення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Safety_EnableLimit(Safety_System_t* safety,
                                  Safety_LimitType_t limit_type,
                                  bool enable);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_SAFETY_H */
