/**
 * @file core.h
 * @brief Основні типи даних та структури бібліотеки ServoCore
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить базові визначення типів, перерахувань та структур,
 * які використовуються по всій бібліотеці керування сервоприводом.
 */

#ifndef SERVOCORE_CORE_H
#define SERVOCORE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Коди статусів виконання операцій
 */
typedef enum {
    SERVO_OK            = 0x00,  /**< Операція виконана успішно */
    SERVO_ERROR         = 0x01,  /**< Загальна помилка */
    SERVO_BUSY          = 0x02,  /**< Пристрій зайнятий */
    SERVO_TIMEOUT       = 0x03,  /**< Таймаут операції */
    SERVO_INVALID       = 0x04,  /**< Некоректні параметри */
    SERVO_NOT_INIT      = 0x05,  /**< Пристрій не ініціалізовано */
    SERVO_ERROR_NULL_PTR = 0x06  /**< Нульовий вказівник */
} Servo_Status_t;

/**
 * @brief Типи двигунів
 */
typedef enum {
    MOTOR_TYPE_DC_PWM     = 0x00,  /**< DC двигун з PWM керуванням */
    MOTOR_TYPE_STEPPER    = 0x01,  /**< Кроковий двигун */
    MOTOR_TYPE_SERVO      = 0x02,  /**< Сервопривід */
    MOTOR_TYPE_BLDC       = 0x03   /**< Безколекторний двигун */
} Motor_Type_t;

/**
 * @brief Типи датчиків положення
 */
typedef enum {
    SENSOR_TYPE_ENCODER_MAG  = 0x00,  /**< Магнітний енкодер */
    SENSOR_TYPE_ENCODER_OPT  = 0x01,  /**< Оптичний енкодер */
    SENSOR_TYPE_POTENTIOMETER = 0x02,  /**< Потенціометр */
    SENSOR_TYPE_RESOLVER     = 0x03   /**< Резольвер */
} Sensor_Type_t;

/**
 * @brief Режими роботи сервоприводу
 */
typedef enum {
    SERVO_MODE_IDLE        = 0x00,  /**< Режим очікування */
    SERVO_MODE_POSITION    = 0x01,  /**< Позиційне керування */
    SERVO_MODE_VELOCITY    = 0x02,  /**< Швидкісне керування */
    SERVO_MODE_TORQUE      = 0x03,  /**< Моментне керування */
    SERVO_MODE_CALIBRATION = 0x04,  /**< Режим калібрування */
    SERVO_MODE_ERROR       = 0xFF   /**< Помилка */
} Servo_Mode_t;

/**
 * @brief Стани сервоприводу
 */
typedef enum {
    SERVO_STATE_UNINIT     = 0x00,  /**< Неініціалізовано */
    SERVO_STATE_READY      = 0x01,  /**< Готовий до роботи */
    SERVO_STATE_RUNNING    = 0x02,  /**< Працює */
    SERVO_STATE_ERROR      = 0x03,  /**< Помилка */
    SERVO_STATE_EMERGENCY  = 0x04   /**< Аварійний режим */
} Servo_State_t;

/**
 * @brief Напрямок обертання двигуна
 */
typedef enum {
    MOTOR_DIR_FORWARD  = 0x00,  /**< Вперед */
    MOTOR_DIR_BACKWARD = 0x01,  /**< Назад */
    MOTOR_DIR_BRAKE    = 0x02   /**< Гальмування */
} Motor_Direction_t;

/**
 * @brief Стани мотора
 */
typedef enum {
    MOTOR_STATE_IDLE    = 0x00,  /**< Мотор не активний */
    MOTOR_STATE_RUNNING = 0x01,  /**< Мотор працює */
    MOTOR_STATE_BRAKING = 0x02,  /**< Мотор гальмує */
    MOTOR_STATE_ERROR   = 0x03   /**< Помилка мотора */
} Motor_State_t;

/**
 * @brief Коди помилок системи
 */
typedef enum {
    ERR_NONE              = 0x0000,  /**< Немає помилок */
    ERR_MOTOR_OVERCURRENT = 0x0001,  /**< Перевантаження по струму */
    ERR_MOTOR_OVERHEAT    = 0x0002,  /**< Перегрів двигуна */
    ERR_MOTOR_STALL       = 0x0003,  /**< Двигун заклинило */
    ERR_SENSOR_LOST       = 0x0010,  /**< Втрата зв'язку з датчиком */
    ERR_SENSOR_INVALID    = 0x0011,  /**< Некоректні дані датчика */
    ERR_POSITION_LIMIT    = 0x0020,  /**< Вихід за межі положення */
    ERR_VELOCITY_LIMIT    = 0x0021,  /**< Перевищення швидкості */
    ERR_WATCHDOG          = 0x0030,  /**< Watchdog таймаут */
    ERR_INIT_FAILED       = 0x0040   /**< Помилка ініціалізації */
} Servo_Error_t;

/**
 * @brief Структура конфігурації осі
 */
typedef struct {
    Motor_Type_t motor_type;    /**< Тип двигуна */
    Sensor_Type_t sensor_type;  /**< Тип датчика положення */
    float max_velocity;         /**< Максимальна швидкість (град/с) */
    float max_acceleration;     /**< Максимальне прискорення (град/с²) */
    float max_current;          /**< Максимальний струм (А) */
    float position_min;         /**< Мінімальне положення (градуси) */
    float position_max;         /**< Максимальне положення (градуси) */
    bool enable_limits;         /**< Увімкнути обмеження положення */
    bool invert_direction;      /**< Інвертувати напрямок */
} Axis_Config_t;

/**
 * @brief Структура стану осі
 */
typedef struct {
    float position;          /**< Поточне положення (градуси) */
    float velocity;          /**< Поточна швидкість (град/с) */
    float target_position;   /**< Цільове положення (градуси) */
    float target_velocity;   /**< Цільова швидкість (град/с) */
    Servo_Mode_t mode;       /**< Поточний режим роботи */
    Servo_State_t state;     /**< Поточний стан */
    Servo_Error_t error;     /**< Код останньої помилки */
} Axis_State_t;

/**
 * @brief Головна структура осі сервоприводу
 */
typedef struct {
    Axis_Config_t config;    /**< Конфігурація осі */
    Axis_State_t  state;     /**< Стан осі */
    void*         motor;     /**< Вказівник на структуру двигуна */
    void*         sensor;    /**< Вказівник на структуру датчика */
    void*         pid;       /**< Вказівник на PID регулятор */
} Servo_Axis_t;

/* Exported constants --------------------------------------------------------*/

/** @brief Версія бібліотеки */
#define SERVOCORE_VERSION_MAJOR  0
#define SERVOCORE_VERSION_MINOR  1
#define SERVOCORE_VERSION_PATCH  0

/** @brief Математичні константи */
#define PI                       3.14159265358979323846f
#define TWO_PI                   6.28318530717958647692f
#define DEG_TO_RAD               0.01745329251994329577f
#define RAD_TO_DEG               57.2957795130823208768f

/* Exported macros -----------------------------------------------------------*/

/**
 * @brief Обмеження значення в межах [min, max]
 */
#define CLAMP(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/**
 * @brief Конвертація градусів у радіани
 */
#define DEG2RAD(deg)  ((deg) * DEG_TO_RAD)

/**
 * @brief Конвертація радіанів у градуси
 */
#define RAD2DEG(rad)  ((rad) * RAD_TO_DEG)

/**
 * @brief Перевірка чи статус OK
 */
#define IS_SERVO_OK(status)  ((status) == SERVO_OK)

/**
 * @brief Перевірка чи статус помилка
 */
#define IS_SERVO_ERROR(status)  ((status) != SERVO_OK)

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CORE_H */
