/**
 * @file position.h
 * @brief Абстрактний інтерфейс датчика положення
 * @author ServoCore Team
 * @date 2025
 *
 * Незалежний від апаратного забезпечення інтерфейс для датчиків положення
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
 * @brief Стан датчика
 */
typedef enum {
    SENSOR_STATE_IDLE      = 0x00,  /**< Очікування */
    SENSOR_STATE_READY     = 0x01,  /**< Готовий */
    SENSOR_STATE_READING   = 0x02,  /**< Читання */
    SENSOR_STATE_ERROR     = 0x03,  /**< Помилка */
    SENSOR_STATE_CALIBRATE = 0x04   /**< Калібрування */
} Sensor_State_t;

/**
 * @brief Параметри датчика
 */
typedef struct {
    Sensor_Type_t type;      /**< Тип датчика */
    uint32_t resolution;     /**< Роздільна здатність (біти/оберт) */
    float min_angle;         /**< Мінімальний кут (градуси) */
    float max_angle;         /**< Максимальний кут (градуси) */
    uint32_t update_rate;    /**< Частота оновлення (Hz) */
    uint32_t sample_rate;    /**< Частота вибірки (Hz) - alias для update_rate */
} Sensor_Params_t;

/**
 * @brief Статистика датчика
 */
typedef struct {
    uint32_t read_count;     /**< Кількість читань */
    uint32_t error_count;    /**< Кількість помилок */
    float last_angle;        /**< Останній кут */
    float last_velocity;     /**< Остання швидкість */
    Sensor_State_t state;    /**< Поточний стан */
} Sensor_Stats_t;

/**
 * @brief Структура інтерфейсу датчика
 */
typedef struct Sensor_Interface Sensor_Interface_t;

struct Sensor_Interface {
    /**
     * @brief Ініціалізація датчика
     */
    Servo_Status_t (*init)(Sensor_Interface_t* self, const Sensor_Params_t* params);

    /**
     * @brief Деініціалізація датчика
     */
    Servo_Status_t (*deinit)(Sensor_Interface_t* self);

    /**
     * @brief Читання кута положення
     */
    Servo_Status_t (*read_angle)(Sensor_Interface_t* self, float* angle);

    /**
     * @brief Читання швидкості обертання
     */
    Servo_Status_t (*read_velocity)(Sensor_Interface_t* self, float* velocity);

    /**
     * @brief Калібрування датчика
     */
    Servo_Status_t (*calibrate)(Sensor_Interface_t* self);

    /**
     * @brief Самотестування датчика
     */
    Servo_Status_t (*self_test)(Sensor_Interface_t* self);

    /**
     * @brief Отримання стану датчика
     */
    Servo_Status_t (*get_state)(Sensor_Interface_t* self, Sensor_State_t* state);

    /**
     * @brief Отримання статистики
     */
    Servo_Status_t (*get_stats)(Sensor_Interface_t* self, Sensor_Stats_t* stats);

    /** @brief Вказівник на конкретну реалізацію */
    void* driver_data;
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація датчика
 */
Servo_Status_t Sensor_Init(Sensor_Interface_t* sensor, const Sensor_Params_t* params);

/**
 * @brief Деініціалізація датчика
 */
Servo_Status_t Sensor_DeInit(Sensor_Interface_t* sensor);

/**
 * @brief Читання кута
 */
Servo_Status_t Sensor_ReadAngle(Sensor_Interface_t* sensor, float* angle);

/**
 * @brief Читання швидкості
 */
Servo_Status_t Sensor_ReadVelocity(Sensor_Interface_t* sensor, float* velocity);

/**
 * @brief Калібрування
 */
Servo_Status_t Sensor_Calibrate(Sensor_Interface_t* sensor);

/**
 * @brief Самотестування
 */
Servo_Status_t Sensor_SelfTest(Sensor_Interface_t* sensor);

/**
 * @brief Отримання стану
 */
Servo_Status_t Sensor_GetState(Sensor_Interface_t* sensor, Sensor_State_t* state);

/**
 * @brief Отримання статистики
 */
Servo_Status_t Sensor_GetStats(Sensor_Interface_t* sensor, Sensor_Stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_POSITION_H */
