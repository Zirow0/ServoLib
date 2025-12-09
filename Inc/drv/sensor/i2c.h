/**
 * @file i2c.h
 * @brief Допоміжні I2C функції для датчиків
 * @author ServoCore Team
 * @date 2025
 *
 * Високорівневі функції для роботи з I2C датчиками:
 * - Автоматичні повтори при помилках
 * - Таймаути та обробка помилок
 * - Зручні утиліти для читання/запису
 */

#ifndef SERVOCORE_DRV_SENSOR_I2C_H
#define SERVOCORE_DRV_SENSOR_I2C_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../hwd/hwd_i2c.h"

/* Exported defines ----------------------------------------------------------*/

/** @brief Кількість повторів за замовчуванням */
#define I2C_SENSOR_DEFAULT_RETRIES    3

/** @brief Затримка між повторами (мс) */
#define I2C_SENSOR_RETRY_DELAY_MS     10

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація I2C для датчика
 */
typedef struct {
    HWD_I2C_Handle_t* i2c_handle;  /**< HWD I2C дескриптор */
    uint16_t device_address;        /**< Адреса пристрою на шині */
    uint8_t max_retries;            /**< Максимальна кількість повторів */
    uint32_t retry_delay_ms;        /**< Затримка між повторами */
} I2C_Sensor_Config_t;

/**
 * @brief Дескриптор I2C датчика
 */
typedef struct {
    I2C_Sensor_Config_t config;     /**< Конфігурація */
    uint32_t error_count;           /**< Лічильник помилок */
    uint32_t success_count;         /**< Лічильник успішних операцій */
    Servo_Status_t last_error;      /**< Остання помилка */
} I2C_Sensor_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація I2C датчика
 *
 * @param handle Вказівник на дескриптор датчика
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_Init(I2C_Sensor_Handle_t* handle,
                               const I2C_Sensor_Config_t* config);

/**
 * @brief Перевірка готовності датчика
 *
 * Виконує перевірку наявності пристрою на I2C шині
 * з автоматичними повторами.
 *
 * @param handle Вказівник на дескриптор датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_IsReady(I2C_Sensor_Handle_t* handle);

/**
 * @brief Читання регістра з повторами
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param data Буфер для даних
 * @param size Розмір даних
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ReadReg(I2C_Sensor_Handle_t* handle,
                                  uint8_t reg_address,
                                  uint8_t* data,
                                  uint16_t size);

/**
 * @brief Запис регістра з повторами
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param data Дані для запису
 * @param size Розмір даних
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_WriteReg(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   const uint8_t* data,
                                   uint16_t size);

/**
 * @brief Читання одного байту з регістра
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ReadByte(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   uint8_t* value);

/**
 * @brief Запис одного байту в регістр
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param value Значення для запису
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_WriteByte(I2C_Sensor_Handle_t* handle,
                                    uint8_t reg_address,
                                    uint8_t value);

/**
 * @brief Читання 16-бітного значення (Big Endian)
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ReadWord16BE(I2C_Sensor_Handle_t* handle,
                                       uint8_t reg_address,
                                       uint16_t* value);

/**
 * @brief Читання 16-бітного значення (Little Endian)
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ReadWord16LE(I2C_Sensor_Handle_t* handle,
                                       uint8_t reg_address,
                                       uint16_t* value);

/**
 * @brief Запис біта в регістр
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param bit_num Номер біта (0-7)
 * @param value Значення біта (0 або 1)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_WriteBit(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   uint8_t bit_num,
                                   uint8_t value);

/**
 * @brief Читання біта з регістра
 *
 * @param handle Вказівник на дескриптор датчика
 * @param reg_address Адреса регістра
 * @param bit_num Номер біта (0-7)
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ReadBit(I2C_Sensor_Handle_t* handle,
                                  uint8_t reg_address,
                                  uint8_t bit_num,
                                  uint8_t* value);

/**
 * @brief Отримання статистики помилок
 *
 * @param handle Вказівник на дескриптор датчика
 * @param error_count Вказівник для збереження кількості помилок
 * @param success_count Вказівник для збереження кількості успіхів
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_GetStats(const I2C_Sensor_Handle_t* handle,
                                   uint32_t* error_count,
                                   uint32_t* success_count);

/**
 * @brief Скидання статистики
 *
 * @param handle Вказівник на дескриптор датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t I2C_Sensor_ResetStats(I2C_Sensor_Handle_t* handle);

#ifdef __cplusplus
}
#endif


#endif /* SERVOCORE_DRV_SENSOR_I2C_H */
