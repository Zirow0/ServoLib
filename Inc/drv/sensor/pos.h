/**
 * @file pos.h
 * @brief Універсальна обгортка датчика положення
 * @author ServoCore Team
 * @date 2025
 *
 * Високорівнева обгортка над будь-яким датчиком положення.
 * Додає фільтрацію, калібрування та додаткові функції.
 */

#ifndef SERVOCORE_DRV_SENSOR_POS_H
#define SERVOCORE_DRV_SENSOR_POS_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../iface/sensor.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація датчика положення
 */
typedef struct {
    Sensor_Interface_t* sensor;      /**< Вказівник на базовий датчик */
    bool enable_calibration;         /**< Увімкнути калібрування */
    float zero_offset;               /**< Нульове зміщення (градуси) */
    float scale_factor;              /**< Коефіцієнт масштабування */
    bool invert_direction;           /**< Інвертувати напрямок */
} Pos_Sensor_Config_t;

/**
 * @brief Дані датчика положення
 */
typedef struct {
    float raw_angle;           /**< Сирий кут з датчика */
    float raw_velocity;        /**< Сира швидкість */
    float calibrated_angle;    /**< Калібрований кут */
    bool data_valid;           /**< Чи дані валідні */
    uint32_t read_count;       /**< Лічильник читань */
    uint32_t error_count;      /**< Лічильник помилок */
} Pos_Sensor_Data_t;

/**
 * @brief Дескриптор датчика положення
 */
typedef struct {
    Pos_Sensor_Config_t config;         /**< Конфігурація */
    Pos_Sensor_Data_t data;             /**< Дані */
} Pos_Sensor_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація датчика положення
 *
 * @param handle Вказівник на дескриптор датчика
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_Init(Pos_Sensor_Handle_t* handle,
                               const Pos_Sensor_Config_t* config);

/**
 * @brief Деініціалізація датчика положення
 *
 * @param handle Вказівник на дескриптор датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_DeInit(Pos_Sensor_Handle_t* handle);

/**
 * @brief Оновлення даних з датчика
 *
 * Виконує читання з датчика, фільтрацію та калібрування.
 * Має викликатися періодично.
 *
 * @param handle Вказівник на дескриптор датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_Update(Pos_Sensor_Handle_t* handle);

/**
 * @brief Отримання поточного кута
 *
 * @param handle Вказівник на дескриптор датчика
 * @param angle Вказівник для збереження кута (градуси)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_GetAngle(const Pos_Sensor_Handle_t* handle, float* angle);

/**
 * @brief Отримання поточної швидкості
 *
 * @param handle Вказівник на дескриптор датчика
 * @param velocity Вказівник для збереження швидкості (град/с)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_GetVelocity(const Pos_Sensor_Handle_t* handle, float* velocity);

/**
 * @brief Отримання сирих (нефільтрованих) даних
 *
 * @param handle Вказівник на дескриптор датчика
 * @param angle Вказівник для збереження кута
 * @param velocity Вказівник для збереження швидкості
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_GetRawData(const Pos_Sensor_Handle_t* handle,
                                     float* angle,
                                     float* velocity);

/**
 * @brief Встановлення нульової точки
 *
 * Встановлює поточне положення як нульову точку.
 *
 * @param handle Вказівник на дескриптор датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_SetZero(Pos_Sensor_Handle_t* handle);

/**
 * @brief Встановлення зміщення
 *
 * @param handle Вказівник на дескриптор датчика
 * @param offset Зміщення в градусах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_SetOffset(Pos_Sensor_Handle_t* handle, float offset);

/**
 * @brief Встановлення коефіцієнта масштабування
 *
 * @param handle Вказівник на дескриптор датчика
 * @param scale Коефіцієнт масштабування
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_SetScale(Pos_Sensor_Handle_t* handle, float scale);

/**
 * @brief Отримання статистики датчика
 *
 * @param handle Вказівник на дескриптор датчика
 * @param read_count Вказівник для збереження кількості читань
 * @param error_count Вказівник для збереження кількості помилок
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Pos_Sensor_GetStats(const Pos_Sensor_Handle_t* handle,
                                   uint32_t* read_count,
                                   uint32_t* error_count);

/**
 * @brief Перевірка валідності даних
 *
 * @param handle Вказівник на дескриптор датчика
 * @return bool true якщо дані валідні
 */
bool Pos_Sensor_IsDataValid(const Pos_Sensor_Handle_t* handle);

#ifdef __cplusplus
}
#endif


#endif /* SERVOCORE_DRV_SENSOR_POS_H */
