/**
 * @file position.h
 * @brief Інтерфейс та базова реалізація датчика положення
 *
 * Спільна логіка (конвертація, velocity, multi-turn) в position.c.
 * Апаратна специфіка — в драйверах (incremental_encoder.c, as5600.c).
 */

#ifndef SERVOCORE_DRV_POSITION_H
#define SERVOCORE_DRV_POSITION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Сирі дані з датчика (заповнює драйвер)
 *
 * angle_rad — абсолютний кут (необмежений, може бути < 0 або >> 2π).
 * Multi-turn відстежується всередині драйвера. position.c нормалізує
 * angle_rad до [0, 2π) для position_rad, і зберігає напряму в
 * absolute_position_rad (з урахуванням angle_offset_rad).
 */
typedef struct {
    float    angle_rad;       /**< Абсолютний кут (рад), необмежений */
    uint32_t timestamp_us;    /**< Мікросекунди (для velocity через derivative) */
    bool     has_velocity;    /**< true — драйвер надає velocity напряму (IC timer) */
    float    velocity_rad_s;  /**< Швидкість рад/с (якщо has_velocity = true) */
    bool     valid;           /**< Валідність даних */
} Position_Raw_Data_t;

/**
 * @brief Внутрішній стан датчика (обчислюється в position.c)
 */
typedef struct {
    float    position_rad;          /**< Поточний кут 0..2π (нормалізований) */
    float    velocity_rad_s;        /**< Кутова швидкість рад/с */
    float    absolute_position_rad; /**< Абсолютний кут з урахуванням angle_offset_rad */
    float    angle_offset_rad;      /**< Зсув нульової точки (встановлюється SetPosition) */

    float    last_position_rad;     /**< Попередній кут (для derivative velocity) */
    uint32_t last_timestamp_us;     /**< Час попереднього Update (для derivative) */

    bool         is_initialized;
    Servo_Error_t last_error;
} Position_Sensor_Data_t;

/**
 * @brief Hardware callbacks від конкретного драйвера
 */
typedef struct {
    /** @brief Ініціалізація апаратури */
    Servo_Status_t (*init)(void* driver_data);

    /** @brief Читання сирих даних (instant — з volatile буфера або HW регістру) */
    Servo_Status_t (*read_raw)(void* driver_data, Position_Raw_Data_t* raw);
} Position_Sensor_HW_Callbacks_t;

/**
 * @brief Інтерфейс датчика положення
 */
typedef struct Position_Sensor_Interface Position_Sensor_Interface_t;

struct Position_Sensor_Interface {
    Position_Sensor_Data_t         data;        /**< Обчислені дані */
    Position_Sensor_HW_Callbacks_t hw;          /**< Hardware callbacks */
    void*                          driver_data; /**< Конкретний драйвер */
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація датчика
 *
 * Викликає hw.init(). Multi-turn відстежується всередині драйвера.
 */
Servo_Status_t Position_Sensor_Init(Position_Sensor_Interface_t* sensor);

/**
 * @brief Оновлення даних датчика
 *
 * Викликає hw.read_raw(), обчислює velocity та multi-turn.
 * Викликати регулярно (1 kHz control loop).
 */
Servo_Status_t Position_Sensor_Update(Position_Sensor_Interface_t* sensor);

/**
 * @brief Отримання позиції (0..360°)
 */
Servo_Status_t Position_Sensor_GetPosition(Position_Sensor_Interface_t* sensor,
                                           float* position_deg);

/**
 * @brief Отримання швидкості (град/с)
 */
Servo_Status_t Position_Sensor_GetVelocity(Position_Sensor_Interface_t* sensor,
                                           float* velocity_deg_s);

/**
 * @brief Отримання абсолютної позиції з урахуванням multi-turn (може бути > 360°)
 */
Servo_Status_t Position_Sensor_GetAbsolutePosition(Position_Sensor_Interface_t* sensor,
                                                   float* abs_position_deg);

/**
 * @brief Встановлення поточної позиції як нової нульової точки
 *
 * @param position_deg Нова позиція (0 = скидання в нуль)
 */
Servo_Status_t Position_Sensor_SetPosition(Position_Sensor_Interface_t* sensor,
                                           float position_deg);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_POSITION_H */
