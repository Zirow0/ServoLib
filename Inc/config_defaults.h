/**
 * @file config_defaults.h
 * @brief Дефолтні значення конфігурації для ServoCore
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить рекомендовані дефолтні значення для типового обладнання.
 * ВСІ параметри можуть бути перевизначені в config_user.h проекту.
 *
 * ВАЖЛИВО: Цей файл включається ПІСЛЯ config_user.h, тому параметри,
 * визначені в config_user.h, мають пріоритет завдяки #ifndef.
 */

#ifndef SERVOCORE_CONFIG_DEFAULTS_H
#define SERVOCORE_CONFIG_DEFAULTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Абсолютні валідаційні межі (можна override для специфічного обладнання!)
 * ===========================================================================*/

/**
 * @brief Абсолютний максимальний струм (mA)
 * @note Для типових DC моторів середньої потужності.
 *       Для промислових моторів може бути збільшено до 20000+ mA
 */
#ifndef ABSOLUTE_MAX_CURRENT
#define ABSOLUTE_MAX_CURRENT 10000U  /* 10A - типові DC мотори */
#endif

/**
 * @brief Абсолютна максимальна температура (°C)
 * @note Для типової автомобільної електроніки.
 *       Для промислової електроніки може бути збільшено до 150°C
 */
#ifndef ABSOLUTE_MAX_TEMP
#define ABSOLUTE_MAX_TEMP 125  /* 125°C - типова електроніка */
#endif

/**
 * @brief Абсолютна максимальна швидкість (град/с)
 * @note Для типових сервоприводів.
 *       Може бути змінено для високошвидкісних систем
 */
#ifndef ABSOLUTE_MAX_VELOCITY
#define ABSOLUTE_MAX_VELOCITY 3600.0f  /* 10 обертів/сек */
#endif

/* =============================================================================
 * Системна конфігурація
 * ===========================================================================*/

/**
 * @brief Частота оновлення контуру керування (Hz)
 * @note Типово 1 kHz для більшості застосувань
 */
#ifndef CONTROL_LOOP_FREQ
#define CONTROL_LOOP_FREQ 1000U  /* 1 kHz */
#endif

/* =============================================================================
 * PWM конфігурація
 * ===========================================================================*/

/**
 * @brief Частота PWM для двигуна (Hz)
 * @note Типово 1 kHz, може бути збільшено до 100 kHz для тихішої роботи
 */
#ifndef PWM_FREQUENCY
#define PWM_FREQUENCY 1000U  /* 1 kHz */
#endif

/**
 * @brief Роздільна здатність PWM (кількість кроків)
 * @note 1000 кроків = 0.1% роздільна здатність
 */
#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION 1000U
#endif

/**
 * @brief Мінімальний duty cycle для старту двигуна
 */
#ifndef PWM_MIN_DUTY
#define PWM_MIN_DUTY 0U
#endif

/* =============================================================================
 * Параметри безпеки двигуна
 * ===========================================================================*/

/**
 * @brief Максимальний струм двигуна (mA)
 * @note Для малопотужних моторів (2A). Для промислових може бути 15000+ mA
 */
#ifndef MOTOR_MAX_CURRENT
#define MOTOR_MAX_CURRENT 2000U  /* 2A */
#endif

/**
 * @brief Поріг струму для захисту (mA)
 * @note Має бути більше MOTOR_MAX_CURRENT
 */
#ifndef MOTOR_OVERCURRENT_LIMIT
#define MOTOR_OVERCURRENT_LIMIT 2500U  /* 2.5A */
#endif

/**
 * @brief Максимальна температура двигуна (°C)
 */
#ifndef MOTOR_MAX_TEMPERATURE
#define MOTOR_MAX_TEMPERATURE 85  /* 85°C */
#endif

/**
 * @brief Час затримки при старті (ms)
 */
#ifndef MOTOR_STARTUP_DELAY_MS
#define MOTOR_STARTUP_DELAY_MS 10
#endif

/**
 * @brief Таймаут watchdog двигуна (ms)
 */
#ifndef MOTOR_WATCHDOG_TIMEOUT
#define MOTOR_WATCHDOG_TIMEOUT 100
#endif

/* =============================================================================
 * Конфігурація сенсора
 * ===========================================================================*/

/**
 * @brief Роздільна здатність енкодера (біт)
 * @note 12 біт для AS5600, 18 біт для AEAT-9922
 */
#ifndef ENCODER_RESOLUTION_BITS
#define ENCODER_RESOLUTION_BITS 12  /* AS5600: 12-bit */
#endif

/**
 * @brief I2C таймаут для датчика (ms)
 */
#ifndef SENSOR_I2C_TIMEOUT
#define SENSOR_I2C_TIMEOUT 10
#endif

/**
 * @brief SPI таймаут для датчика (ms)
 */
#ifndef SENSOR_SPI_TIMEOUT
#define SENSOR_SPI_TIMEOUT 10
#endif

/**
 * @brief Кількість спроб читання при помилці
 */
#ifndef SENSOR_READ_RETRIES
#define SENSOR_READ_RETRIES 3
#endif

/**
 * @brief Час очікування після ініціалізації датчика (ms)
 */
#ifndef SENSOR_INIT_DELAY_MS
#define SENSOR_INIT_DELAY_MS 50
#endif

/* =============================================================================
 * Конфігурація керування положенням
 * ===========================================================================*/

/**
 * @brief Мінімальне положення за замовчуванням (градуси)
 */
#ifndef DEFAULT_MIN_POSITION
#define DEFAULT_MIN_POSITION 0.0f
#endif

/**
 * @brief Максимальне положення за замовчуванням (градуси)
 */
#ifndef DEFAULT_MAX_POSITION
#define DEFAULT_MAX_POSITION 360.0f
#endif

/**
 * @brief Максимальна швидкість за замовчуванням (град/с)
 */
#ifndef DEFAULT_MAX_VELOCITY
#define DEFAULT_MAX_VELOCITY 180.0f  /* 0.5 обертів/сек */
#endif

/**
 * @brief Максимальне прискорення за замовчуванням (град/с²)
 */
#ifndef DEFAULT_MAX_ACCELERATION
#define DEFAULT_MAX_ACCELERATION 360.0f  /* 1 оберт/с² */
#endif

/**
 * @brief Зона нечутливості для позиційного керування (градуси)
 */
#ifndef POSITION_DEADBAND
#define POSITION_DEADBAND 0.1f
#endif

/**
 * @brief Порогова помилка для визначення досягнення цілі (градуси)
 */
#ifndef POSITION_ERROR_THRESHOLD
#define POSITION_ERROR_THRESHOLD 0.5f
#endif

/* =============================================================================
 * Конфігурація PID регулятора
 * ===========================================================================*/

/**
 * @brief Коефіцієнт P за замовчуванням
 * @note Має бути налаштовано для конкретної системи
 */
#ifndef DEFAULT_PID_KP
#define DEFAULT_PID_KP 1.0f
#endif

/**
 * @brief Коефіцієнт I за замовчуванням
 */
#ifndef DEFAULT_PID_KI
#define DEFAULT_PID_KI 0.1f
#endif

/**
 * @brief Коефіцієнт D за замовчуванням
 */
#ifndef DEFAULT_PID_KD
#define DEFAULT_PID_KD 0.05f
#endif

/**
 * @brief Максимальне значення інтегральної складової (anti-windup)
 */
#ifndef PID_INTEGRAL_MAX
#define PID_INTEGRAL_MAX 100.0f
#endif

/**
 * @brief Мінімальне значення інтегральної складової
 */
#ifndef PID_INTEGRAL_MIN
#define PID_INTEGRAL_MIN -100.0f
#endif

/**
 * @brief Максимальний вихід PID
 */
#ifndef PID_OUTPUT_MAX
#define PID_OUTPUT_MAX 100.0f
#endif

/**
 * @brief Мінімальний вихід PID
 */
#ifndef PID_OUTPUT_MIN
#define PID_OUTPUT_MIN -100.0f
#endif

/* =============================================================================
 * Конфігурація безпеки
 * ===========================================================================*/

/**
 * @brief Увімкнути перевірку меж положення
 */
#ifndef ENABLE_POSITION_LIMITS
#define ENABLE_POSITION_LIMITS 1
#endif

/**
 * @brief Увімкнути обмеження швидкості
 */
#ifndef ENABLE_VELOCITY_LIMITS
#define ENABLE_VELOCITY_LIMITS 1
#endif

/**
 * @brief Увімкнути струмовий захист
 */
#ifndef ENABLE_CURRENT_PROTECTION
#define ENABLE_CURRENT_PROTECTION 1
#endif

/**
 * @brief Увімкнути температурний захист
 */
#ifndef ENABLE_THERMAL_PROTECTION
#define ENABLE_THERMAL_PROTECTION 1
#endif

/**
 * @brief Увімкнути watchdog таймер
 */
#ifndef ENABLE_WATCHDOG
#define ENABLE_WATCHDOG 1
#endif

/**
 * @brief Час аварійного зупинки (ms)
 */
#ifndef EMERGENCY_STOP_TIME_MS
#define EMERGENCY_STOP_TIME_MS 10
#endif

/* =============================================================================
 * Генерація траєкторій
 * ===========================================================================*/

/**
 * @brief Увімкнути генератор траєкторій
 */
#ifndef ENABLE_TRAJECTORY_GEN
#define ENABLE_TRAJECTORY_GEN 1
#endif

/**
 * @brief Тип траєкторії за замовчуванням (0=лінійна, 1=S-крива)
 */
#ifndef DEFAULT_TRAJECTORY_TYPE
#define DEFAULT_TRAJECTORY_TYPE 1
#endif

/**
 * @brief Максимальний ривок (град/с³)
 */
#ifndef MAX_JERK
#define MAX_JERK 1000.0f
#endif

/* =============================================================================
 * Налагодження та логування
 * ===========================================================================*/

/**
 * @brief Увімкнути налагоджувальні повідомлення
 */
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 1
#endif

/**
 * @brief Увімкнути логування помилок
 */
#ifndef ENABLE_ERROR_LOGGING
#define ENABLE_ERROR_LOGGING 1
#endif

/**
 * @brief Розмір буфера логування помилок
 */
#ifndef ERROR_LOG_BUFFER_SIZE
#define ERROR_LOG_BUFFER_SIZE 32
#endif

/**
 * @brief Увімкнути вимірювання продуктивності
 */
#ifndef ENABLE_PERFORMANCE_STATS
#define ENABLE_PERFORMANCE_STATS 0
#endif

/* =============================================================================
 * Конфігурація пам'яті
 * ===========================================================================*/

/**
 * @brief Максимальна кількість осей
 */
#ifndef MAX_AXES
#define MAX_AXES 6
#endif

/**
 * @brief Розмір стеку для кожної осі (байт)
 */
#ifndef AXIS_STACK_SIZE
#define AXIS_STACK_SIZE 1024
#endif

/**
 * @brief Використовувати статичну пам'ять (без malloc)
 */
#ifndef USE_STATIC_MEMORY
#define USE_STATIC_MEMORY 1
#endif

/* =============================================================================
 * Умовна компіляція функціональності
 * ===========================================================================*/

/**
 * @brief Підтримка режиму швидкісного керування
 */
#ifndef SUPPORT_VELOCITY_MODE
#define SUPPORT_VELOCITY_MODE 1
#endif

/**
 * @brief Підтримка режиму моментного керування
 */
#ifndef SUPPORT_TORQUE_MODE
#define SUPPORT_TORQUE_MODE 0
#endif

/**
 * @brief Підтримка автоматичного калібрування
 */
#ifndef SUPPORT_AUTO_CALIBRATION
#define SUPPORT_AUTO_CALIBRATION 1
#endif

/**
 * @brief Підтримка збереження налаштувань
 */
#ifndef SUPPORT_SETTINGS_SAVE
#define SUPPORT_SETTINGS_SAVE 0
#endif

/* =============================================================================
 * Конфігурація гальм
 * ===========================================================================*/

/**
 * @brief Затримка відпускання гальм (ms)
 */
#ifndef BRAKE_RELEASE_DELAY_MS
#define BRAKE_RELEASE_DELAY_MS 100
#endif

/**
 * @brief Таймаут блокування гальм (ms)
 */
#ifndef BRAKE_ENGAGE_TIMEOUT_MS
#define BRAKE_ENGAGE_TIMEOUT_MS 3000
#endif

/**
 * @brief Таймаут бездіяльності для автоматичного гальмування (ms)
 */
#ifndef BRAKE_IDLE_TIMEOUT_MS
#define BRAKE_IDLE_TIMEOUT_MS 5000
#endif

/* =============================================================================
 * Обчислені константи (НЕ override!)
 * ===========================================================================*/

/** @brief Період оновлення контуру керування (ms) */
#define CONTROL_LOOP_PERIOD_MS (1000U / CONTROL_LOOP_FREQ)

/** @brief Кількість позицій на оберт */
#define ENCODER_COUNTS_PER_REV (1 << ENCODER_RESOLUTION_BITS)

/** @brief Максимальне значення duty cycle */
#define PWM_MAX_DUTY PWM_RESOLUTION

/* =============================================================================
 * Валідація розумності параметрів
 * ===========================================================================*/

#if (CONTROL_LOOP_FREQ > 10000)
    #error "Control loop frequency too high! Maximum is 10 kHz"
#endif

#if (CONTROL_LOOP_FREQ == 0)
    #error "CONTROL_LOOP_FREQ cannot be zero"
#endif

#if (PWM_FREQUENCY > 100000)
    #error "PWM frequency too high! Maximum is 100 kHz"
#endif

#if (MAX_AXES > 8)
    #error "Too many axes! Maximum is 8"
#endif

#if (ENCODER_RESOLUTION_BITS > 20)
    #error "Encoder resolution too high! Maximum is 20 bits"
#endif

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CONFIG_DEFAULTS_H */
