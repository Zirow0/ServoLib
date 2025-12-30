/**
 * @file position.h
 * @brief Інтерфейс та базова реалізація датчика положення
 * @author ServoCore Team
 * @date 2025
 *
 * Універсальний інтерфейс для датчиків положення з Hardware Callbacks Pattern.
 * Спільна логіка (конвертація, velocity, multi-turn, prediction) в position.c,
 * апаратна специфіка (SPI/I2C read) в драйверах (aeat9922.c, as5600.c).
 */

#ifndef SERVOCORE_DRV_POSITION_H
#define SERVOCORE_DRV_POSITION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Можливості датчика позиції
 */
typedef enum {
    POSITION_CAP_ABSOLUTE    = 0x01,  /**< Абсолютна позиція (AS5600, AEAT9922) */
    POSITION_CAP_INCREMENTAL = 0x02,  /**< Інкрементальна (Quadrature encoder) */
    POSITION_CAP_VELOCITY    = 0x04,  /**< Датчик надає velocity */
    POSITION_CAP_INTERRUPT   = 0x08,  /**< Interrupt-driven (Hall sensors) */
    POSITION_CAP_MULTITURN   = 0x10   /**< Multi-turn tracking */
} Position_Capabilities_t;

/**
 * @brief Сирі дані з датчика (заповнює драйвер)
 */
typedef struct {
    uint32_t raw_position;      /**< Сире значення (0 до 2^resolution-1) */
    uint32_t timestamp_us;      /**< Мікросекунди (для velocity) */
    bool has_velocity;          /**< Чи датчик надає готову velocity */
    float raw_velocity;         /**< Якщо датчик сам обчислює */
    bool valid;                 /**< Валідність даних */
} Position_Raw_Data_t;

/**
 * @brief Параметри датчика позиції
 */
typedef struct {
    Sensor_Type_t type;         /**< Тип датчика */
    uint32_t resolution_bits;   /**< Роздільна здатність (12, 18 біт) */
    float min_angle;            /**< Мінімальний кут (градуси) */
    float max_angle;            /**< Максимальний кут (градуси) */
    uint32_t update_rate;       /**< Частота оновлення (Hz) */
} Position_Params_t;

/**
 * @brief Статистика датчика позиції
 */
typedef struct {
    uint32_t update_count;      /**< Кількість оновлень */
    uint32_t error_count;       /**< Кількість помилок */
    float last_position_deg;    /**< Остання позиція */
    float last_velocity_deg_s;  /**< Остання швидкість */
    bool is_calibrated;         /**< Стан калібрування */
} Position_Stats_t;

/**
 * @brief Дані датчика позиції (обробляються в position.c)
 */
typedef struct {
    // Поточні дані (обчислені)
    float position_deg;           /**< Позиція 0-360° */
    float velocity_deg_s;         /**< Швидкість (град/с) */

    // Multi-turn tracking
    int32_t revolution_count;     /**< Кількість повних обертів */
    float absolute_position_deg;  /**< position + revolutions*360 */

    // Для velocity обчислення
    uint32_t last_timestamp_us;   /**< Час останнього update */
    float last_position_deg;      /**< Попередня позиція */

    // Prediction (екстраполяція між updates)
    float predicted_position_deg;   /**< Передбачена позиція */
    uint32_t prediction_timestamp_us; /**< Час prediction */

    // Статус
    Position_Stats_t stats;       /**< Статистика */
    bool is_initialized;          /**< Прапорець ініціалізації */
    bool is_calibrated;           /**< Прапорець калібрування */

    Servo_Error_t last_error;     /**< Код останньої помилки */
} Position_Sensor_Data_t;

/**
 * @brief Hardware callbacks для драйвера датчика позиції
 *
 * Конкретний драйвер (AEAT9922, AS5600) надає ці функції.
 */
typedef struct {
    /** @brief Ініціалізація апаратури (SPI, I2C) */
    Servo_Status_t (*init)(void* driver_data, const Position_Params_t* params);

    /** @brief Деініціалізація апаратури */
    Servo_Status_t (*deinit)(void* driver_data);

    /** @brief Читання сирих даних з датчика (БЕЗ конвертації!) */
    Servo_Status_t (*read_raw)(void* driver_data, Position_Raw_Data_t* raw);

    /** @brief Калібрування датчика */
    Servo_Status_t (*calibrate)(void* driver_data);

    /** @brief Callback для interrupt (опціонально) */
    void (*notify_callback)(void* driver_data);
} Position_Sensor_HW_Callbacks_t;

/**
 * @brief Структура інтерфейсу датчика позиції
 *
 * Містить Position_Sensor_Data_t для спільної логіки та hardware callbacks
 * для апаратної специфіки.
 */
typedef struct Position_Sensor_Interface Position_Sensor_Interface_t;

struct Position_Sensor_Interface {
    /** @brief Дані датчика (логіка, стан, статистика) */
    Position_Sensor_Data_t data;

    /** @brief Hardware callbacks від конкретного драйвера */
    Position_Sensor_HW_Callbacks_t hw;

    /** @brief Можливості датчика */
    Position_Capabilities_t capabilities;

    /** @brief Роздільна здатність (12, 18 біт) */
    uint32_t resolution_bits;

    /** @brief Чи потрібне калібрування (incremental = true) */
    bool requires_calibration;

    /** @brief Вказівник на конкретну реалізацію драйвера */
    void* driver_data;
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація датчика позиції
 *
 * Викликає hardware init callback та ініціалізує базові дані.
 *
 * @param sensor Вказівник на Position_Sensor_Interface_t
 * @param params Параметри датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_Init(Position_Sensor_Interface_t* sensor,
                                   const Position_Params_t* params);

/**
 * @brief Деініціалізація датчика позиції
 *
 * @param sensor Вказівник на Position_Sensor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_DeInit(Position_Sensor_Interface_t* sensor);

/**
 * @brief Оновлення датчика позиції
 *
 * Викликає hw.read_raw(), обчислює position, velocity, multi-turn, prediction.
 * КРИТИЧНО: Викликати регулярно (наприклад, у control loop 1 kHz)
 *
 * @param sensor Вказівник на Position_Sensor_Interface_t
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_Update(Position_Sensor_Interface_t* sensor);

/**
 * @brief Отримання позиції (0-360°)
 *
 * @param sensor Вказівник на інтерфейс
 * @param position_deg Вказівник для збереження позиції
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_GetPosition(Position_Sensor_Interface_t* sensor,
                                          float* position_deg);

/**
 * @brief Отримання швидкості (град/с)
 *
 * @param sensor Вказівник на інтерфейс
 * @param velocity_deg_s Вказівник для збереження швидкості
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_GetVelocity(Position_Sensor_Interface_t* sensor,
                                          float* velocity_deg_s);

/**
 * @brief Отримання абсолютної позиції (з multi-turn)
 *
 * @param sensor Вказівник на інтерфейс
 * @param abs_position_deg Вказівник для збереження (може бути > 360°)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_GetAbsolutePosition(Position_Sensor_Interface_t* sensor,
                                                  float* abs_position_deg);

/**
 * @brief Отримання передбаченої позиції (prediction)
 *
 * Повертає екстрапольовану позицію на поточний момент між updates.
 *
 * @param sensor Вказівник на інтерфейс
 * @param position_deg Вказівник для збереження
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_GetPredictedPosition(Position_Sensor_Interface_t* sensor,
                                                    float* position_deg);

/**
 * @brief Встановлення позиції (zero reset)
 *
 * Встановлює поточну позицію як нову нульову точку.
 *
 * @param sensor Вказівник на інтерфейс
 * @param position_deg Нова позиція (градуси)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_SetPosition(Position_Sensor_Interface_t* sensor,
                                          float position_deg);

/**
 * @brief Калібрування датчика
 *
 * Викликає hardware calibrate callback (якщо є).
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_Calibrate(Position_Sensor_Interface_t* sensor);

/**
 * @brief Отримання статистики
 *
 * @param sensor Вказівник на інтерфейс
 * @param stats Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Position_Sensor_GetStats(Position_Sensor_Interface_t* sensor,
                                       Position_Stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_POSITION_H */
