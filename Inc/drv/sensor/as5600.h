/**
 * @file as5600.h
 * @brief Драйвер магнітного енкодера AS5600
 * @author ServoCore Team
 * @date 2025
 *
 * Драйвер для 12-bit магнітного енкодера AS5600 (I2C)
 * Роздільна здатність: 4096 позицій на оберт (0.088°)
 */

#ifndef SERVOCORE_DRV_SENSOR_AS5600_H
#define SERVOCORE_DRV_SENSOR_AS5600_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../iface/sensor.h"
#include "../../hwd/hwd_i2c.h"

/* Exported constants --------------------------------------------------------*/

/** @brief I2C адреса AS5600 (7-bit) */
#define AS5600_ADDRESS          0x36
#define AS5600_I2C_ADDRESS      0x36  /**< Alias */

/** @brief Регістри AS5600 */
#define AS5600_REG_ZMCO         0x00  /**< Кількість записів ZPOS/MPOS */
#define AS5600_REG_ZPOS_H       0x01  /**< Zero position high byte */
#define AS5600_REG_ZPOS_L       0x02  /**< Zero position low byte */
#define AS5600_REG_MPOS_H       0x03  /**< Maximum position high byte */
#define AS5600_REG_MPOS_L       0x04  /**< Maximum position low byte */
#define AS5600_REG_MANG_H       0x05  /**< Maximum angle high byte */
#define AS5600_REG_MANG_L       0x06  /**< Maximum angle low byte */
#define AS5600_REG_CONF_H       0x07  /**< Configuration high byte */
#define AS5600_REG_CONF_L       0x08  /**< Configuration low byte */
#define AS5600_REG_RAW_ANGLE_H  0x0C  /**< Raw angle high byte */
#define AS5600_REG_RAW_ANGLE_L  0x0D  /**< Raw angle low byte */
#define AS5600_REG_ANGLE_H      0x0E  /**< Angle high byte */
#define AS5600_REG_ANGLE_L      0x0F  /**< Angle low byte */
#define AS5600_REG_STATUS       0x0B  /**< Status register */
#define AS5600_REG_AGC          0x1A  /**< Automatic Gain Control */
#define AS5600_REG_MAGNITUDE_H  0x1B  /**< Magnitude high byte */
#define AS5600_REG_MAGNITUDE_L  0x1C  /**< Magnitude low byte */
#define AS5600_REG_BURN         0xFF  /**< Burn command */

/** @brief Біти статусу */
#define AS5600_STATUS_MH        (1 << 3)  /**< Magnet detected */
#define AS5600_STATUS_ML        (1 << 4)  /**< AGC minimum overflow */
#define AS5600_STATUS_MD        (1 << 5)  /**< AGC maximum overflow */

/** @brief Максимальне значення (12-bit) */
#define AS5600_MAX_VALUE        4095

/** @brief Роздільна здатність в градусах */
#define AS5600_RESOLUTION       (360.0f / 4096.0f)  // 0.088°

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан магніту
 */
typedef enum {
    AS5600_MAGNET_NOT_DETECTED = 0,  /**< Магніт не виявлено */
    AS5600_MAGNET_TOO_WEAK     = 1,  /**< Магніт слабкий */
    AS5600_MAGNET_GOOD         = 2,  /**< Магніт в нормі */
    AS5600_MAGNET_TOO_STRONG   = 3   /**< Магніт занадто сильний */
} AS5600_MagnetStatus_t;

/**
 * @brief Конфігурація AS5600
 */
typedef struct {
    HWD_I2C_Handle_t* i2c;   /**< I2C дескриптор */
    uint16_t address;        /**< I2C адреса (зазвичай 0x36) */
    bool use_raw_angle;      /**< Використовувати сирий кут (без ZPOS/MPOS) */
} AS5600_Config_t;

/**
 * @brief Дані AS5600
 */
typedef struct {
    uint16_t raw_angle;          /**< Сирий кут (0-4095) */
    uint16_t angle;              /**< Кут з урахуванням ZPOS/MPOS */
    float angle_degrees;         /**< Кут в градусах */
    float velocity;              /**< Швидкість (град/с) */
    uint8_t agc;                 /**< AGC значення */
    uint16_t magnitude;          /**< Magnitude */
    AS5600_MagnetStatus_t magnet;/**< Стан магніту */

    uint16_t last_raw_angle;     /**< Попередній сирий кут */
    uint32_t last_read_time;     /**< Час останнього читання */
    int32_t revolution_count;    /**< Лічильник обертів */
} AS5600_Data_t;

/**
 * @brief Драйвер AS5600
 */
typedef struct {
    Sensor_Interface_t interface;  /**< Інтерфейс датчика */
    AS5600_Config_t config;        /**< Конфігурація */
    AS5600_Data_t data;            /**< Дані */
    Sensor_Stats_t stats;          /**< Статистика */
    bool is_initialized;           /**< Прапорець ініціалізації */
} AS5600_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера AS5600
 *
 * @param driver Вказівник на драйвер
 * @param config Конфігурація
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_Create(AS5600_Driver_t* driver, const AS5600_Config_t* config);

/**
 * @brief Ініціалізація AS5600
 *
 * @param driver Вказівник на драйвер
 * @param params Параметри датчика
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_Init(AS5600_Driver_t* driver, const Sensor_Params_t* params);

/**
 * @brief Деініціалізація AS5600
 *
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_DeInit(AS5600_Driver_t* driver);

/**
 * @brief Читання сирого кута
 *
 * @param driver Вказівник на драйвер
 * @param raw_angle Вказівник для збереження сирого кута (0-4095)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadRawAngle(AS5600_Driver_t* driver, uint16_t* raw_angle);

/**
 * @brief Читання кута в градусах
 *
 * @param driver Вказівник на драйвер
 * @param angle Вказівник для збереження кута (0-360°)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadAngle(AS5600_Driver_t* driver, float* angle);

/**
 * @brief Читання швидкості
 *
 * @param driver Вказівник на драйвер
 * @param velocity Вказівник для збереження швидкості (град/с)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadVelocity(AS5600_Driver_t* driver, float* velocity);

/**
 * @brief Оновлення всіх даних
 *
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_Update(AS5600_Driver_t* driver);

/**
 * @brief Перевірка статусу магніту
 *
 * @param driver Вказівник на драйвер
 * @param status Вказівник для збереження статусу
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_GetMagnetStatus(AS5600_Driver_t* driver,
                                      AS5600_MagnetStatus_t* status);

/**
 * @brief Читання AGC (Automatic Gain Control)
 *
 * @param driver Вказівник на драйвер
 * @param agc Вказівник для збереження AGC
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadAGC(AS5600_Driver_t* driver, uint8_t* agc);

/**
 * @brief Читання magnitude
 *
 * @param driver Вказівник на драйвер
 * @param magnitude Вказівник для збереження magnitude
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadMagnitude(AS5600_Driver_t* driver, uint16_t* magnitude);

/**
 * @brief Встановлення нульової позиції (ZPOS)
 *
 * @param driver Вказівник на драйвер
 * @param position Позиція (0-4095)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_SetZeroPosition(AS5600_Driver_t* driver, uint16_t position);

/**
 * @brief Встановлення максимальної позиції (MPOS)
 *
 * @param driver Вказівник на драйвер
 * @param position Позиція (0-4095)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_SetMaxPosition(AS5600_Driver_t* driver, uint16_t position);

/**
 * @brief Отримання інтерфейсу датчика
 *
 * @param driver Вказівник на драйвер
 * @return Sensor_Interface_t* Вказівник на інтерфейс
 */
Sensor_Interface_t* AS5600_GetInterface(AS5600_Driver_t* driver);

/**
 * @brief Самотестування
 *
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_SelfTest(AS5600_Driver_t* driver);

#ifdef __cplusplus
}
#endif


#endif /* SERVOCORE_DRV_SENSOR_AS5600_H */
