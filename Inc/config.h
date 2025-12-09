/**
 * @file config.h
 * @brief Глобальні конфігураційні параметри бібліотеки ServoCore
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить налаштування за замовчуванням та параметри компіляції
 * для всієї системи керування сервоприводом.
 */

#ifndef SERVOCORE_CONFIG_H
#define SERVOCORE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "core.h"  /* Базові типи та константи */

/* Hardware Configuration ----------------------------------------------------*/

/** @brief Частота системного таймера (Hz) */
#define SYSTEM_CLOCK_FREQ        100000000U  /* 100 MHz для STM32F411 */

/** @brief Частота оновлення контуру керування (Hz) */
#define CONTROL_LOOP_FREQ        1000U       /* 1 kHz */

/** @brief Період оновлення контуру керування (ms) */
#define CONTROL_LOOP_PERIOD_MS   (1000U / CONTROL_LOOP_FREQ)

/* PWM Configuration ---------------------------------------------------------*/

/** @brief Частота PWM для двигуна (Hz) */
#define PWM_FREQUENCY            1000U       /* 1 kHz */

/** @brief Роздільна здатність PWM (кількість кроків) */
#define PWM_RESOLUTION           1000U

/** @brief Максимальне значення duty cycle (0-1000) */
#define PWM_MAX_DUTY             PWM_RESOLUTION

/** @brief Мінімальний duty cycle для старту двигуна */
#define PWM_MIN_DUTY             0U

/* Motor Configuration -------------------------------------------------------*/

/** @brief Максимальний струм двигуна (mA) */
#define MOTOR_MAX_CURRENT        2000U

/** @brief Поріг струму для захисту (mA) */
#define MOTOR_OVERCURRENT_LIMIT  2500U

/** @brief Максимальна температура двигуна (°C) */
#define MOTOR_MAX_TEMPERATURE    85

/** @brief Час затримки при старті (ms) */
#define MOTOR_STARTUP_DELAY_MS   10

/** @brief Таймаут watchdog двигуна (ms) */
#define MOTOR_WATCHDOG_TIMEOUT   100

/* Sensor Configuration ------------------------------------------------------*/

/** @brief Роздільна здатність енкодера AS5600 (біт) */
#define ENCODER_RESOLUTION_BITS  12

/** @brief Кількість позицій на оберт */
#define ENCODER_COUNTS_PER_REV   (1 << ENCODER_RESOLUTION_BITS)  /* 4096 */

/** @brief I2C таймаут для датчика (ms) */
#define SENSOR_I2C_TIMEOUT       10

/** @brief Кількість спроб читання при помилці */
#define SENSOR_READ_RETRIES      3

/** @brief Час очікування після ініціалізації датчика (ms) */
#define SENSOR_INIT_DELAY_MS     50

/* Position Control Configuration --------------------------------------------*/

/** @brief Мінімальне положення за замовчуванням (градуси) */
#define DEFAULT_MIN_POSITION     0.0f

/** @brief Максимальне положення за замовчуванням (градуси) */
#define DEFAULT_MAX_POSITION     360.0f

/** @brief Максимальна швидкість за замовчуванням (град/с) */
#define DEFAULT_MAX_VELOCITY     180.0f

/** @brief Максимальне прискорення за замовчуванням (град/с²) */
#define DEFAULT_MAX_ACCELERATION 360.0f

/** @brief Зона нечутливості для позиційного керування (градуси) */
#define POSITION_DEADBAND        0.1f

/** @brief Порогова помилка для визначення досягнення цілі (градуси) */
#define POSITION_ERROR_THRESHOLD 0.5f

/* PID Controller Configuration ----------------------------------------------*/

/** @brief Коефіцієнт P за замовчуванням */
#define DEFAULT_PID_KP           1.0f

/** @brief Коефіцієнт I за замовчуванням */
#define DEFAULT_PID_KI           0.1f

/** @brief Коефіцієнт D за замовчуванням */
#define DEFAULT_PID_KD           0.05f

/** @brief Максимальне значення інтегральної складової (anti-windup) */
#define PID_INTEGRAL_MAX         100.0f

/** @brief Мінімальне значення інтегральної складової */
#define PID_INTEGRAL_MIN         -100.0f

/** @brief Максимальний вихід PID */
#define PID_OUTPUT_MAX           100.0f

/** @brief Мінімальний вихід PID */
#define PID_OUTPUT_MIN           -100.0f

/* Safety Configuration ------------------------------------------------------*/

/** @brief Увімкнути перевірку меж положення */
#define ENABLE_POSITION_LIMITS   1

/** @brief Увімкнути обмеження швидкості */
#define ENABLE_VELOCITY_LIMITS   1

/** @brief Увімкнути струмовий захист */
#define ENABLE_CURRENT_PROTECTION 1

/** @brief Увімкнути температурний захист */
#define ENABLE_THERMAL_PROTECTION 1

/** @brief Увімкнути watchdog таймер */
#define ENABLE_WATCHDOG          1

/** @brief Час аварійного зупинки (ms) */
#define EMERGENCY_STOP_TIME_MS   10

/* Trajectory Generation -----------------------------------------------------*/

/** @brief Увімкнути генератор траєкторій */
#define ENABLE_TRAJECTORY_GEN    1

/** @brief Тип траєкторії за замовчуванням (0=лінійна, 1=S-крива) */
#define DEFAULT_TRAJECTORY_TYPE  1

/** @brief Максимальний ривок (град/с³) */
#define MAX_JERK                 1000.0f

/* Debug and Logging ---------------------------------------------------------*/

/** @brief Увімкнути налагоджувальні повідомлення */
#define ENABLE_DEBUG             1

/** @brief Увімкнути логування помилок */
#define ENABLE_ERROR_LOGGING     1

/** @brief Розмір буфера логування помилок */
#define ERROR_LOG_BUFFER_SIZE    32

/** @brief Увімкнути вимірювання продуктивності */
#define ENABLE_PERFORMANCE_STATS 0

/* Memory Configuration ------------------------------------------------------*/

/** @brief Максимальна кількість осей */
#define MAX_AXES                 6

/** @brief Розмір стеку для кожної осі (байт) */
#define AXIS_STACK_SIZE          1024

/** @brief Використовувати статичну пам'ять (без malloc) */
#define USE_STATIC_MEMORY        1

/* Conditional Compilation ---------------------------------------------------*/

/** @brief Підтримка режиму швидкісного керування */
#define SUPPORT_VELOCITY_MODE    1

/** @brief Підтримка режиму моментного керування */
#define SUPPORT_TORQUE_MODE      0

/** @brief Підтримка автоматичного калібрування */
#define SUPPORT_AUTO_CALIBRATION 1

/** @brief Підтримка збереження налаштувань */
#define SUPPORT_SETTINGS_SAVE    0

/* Validation ----------------------------------------------------------------*/

#if (CONTROL_LOOP_FREQ > 10000)
    #error "Control loop frequency too high! Maximum is 10 kHz"
#endif

#if (PWM_FREQUENCY > 100000)
    #error "PWM frequency too high! Maximum is 100 kHz"
#endif

#if (MAX_AXES > 8)
    #error "Too many axes! Maximum is 8"
#endif

/* Note: Cannot use floating-point validation in preprocessor directives */
/* DEFAULT_MAX_VELOCITY validation must be done at runtime if needed */

/* Platform-specific includes ------------------------------------------------*/

#ifdef STM32F411xE
    #include "stm32f4xx_hal.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CONFIG_H */
