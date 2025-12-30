/**
 * @file as5600.h
 * @brief Драйвер магнітного енкодера AS5600
 * @author ServoCore Team
 * @date 2025
 *
 * Драйвер для 12-bit магнітного енкодера AS5600 (I2C).
 * Роздільна здатність: 4096 позицій на оберт (0.088°).
 * Реалізує Hardware Callbacks Pattern - тільки апаратні операції (I2C),
 * вся логіка (конвертація, velocity, multi-turn) в position.c.
 */

#ifndef SERVOCORE_DRV_AS5600_H
#define SERVOCORE_DRV_AS5600_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "position.h"
#include "../../hwd/hwd_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан магніту AS5600
 */
typedef enum {
    AS5600_MAGNET_NOT_DETECTED = 0,  /**< Магніт не виявлено */
    AS5600_MAGNET_TOO_WEAK     = 1,  /**< Магніт слабкий (AGC < min) */
    AS5600_MAGNET_GOOD         = 2,  /**< Магніт в нормі */
    AS5600_MAGNET_TOO_STRONG   = 3   /**< Магніт занадто сильний (AGC > max) */
} AS5600_MagnetStatus_t;

/**
 * @brief Конфігурація AS5600
 */
typedef struct {
    // I2C конфігурація
    HWD_I2C_Config_t i2c_config;

    // Параметри читання
    bool use_raw_angle;      /**< true = RAW_ANGLE (без ZPOS/MPOS), false = ANGLE */

} AS5600_Config_t;

/**
 * @brief Структура драйвера AS5600
 *
 * Містить тільки апаратну специфіку (I2C, статус AS5600).
 * Вся логіка (position, velocity, multi-turn) в Position_Sensor_Interface_t.
 */
typedef struct {
    Position_Sensor_Interface_t interface;  /**< Універсальний інтерфейс (ПЕРШИЙ!) */
    AS5600_Config_t config;                 /**< Конфігурація енкодера */
    HWD_I2C_Handle_t i2c_handle;            /**< Дескриптор I2C */

    // ТІЛЬКИ AS5600-специфічне
    AS5600_MagnetStatus_t magnet_status;    /**< Стан магніту */
    uint8_t agc;                            /**< Automatic Gain Control */
    uint16_t magnitude;                     /**< Сила магнітного поля */
    uint32_t error_count;                   /**< Лічильник помилок I2C */

} AS5600_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера AS5600
 *
 * Ініціалізує структуру драйвера з конфігурацією та прив'язує hardware callbacks.
 * Після створення використовуйте &driver->interface для доступу до Position_Sensor_Interface_t.
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_Create(AS5600_Driver_t* driver,
                              const AS5600_Config_t* config);

/**
 * @brief Зчитування статусу магніту
 *
 * Читає регістр STATUS та оновлює magnet_status, AGC, magnitude.
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadMagnetStatus(AS5600_Driver_t* driver);

/**
 * @brief Читання AGC (Automatic Gain Control)
 *
 * @param driver Вказівник на структуру драйвера
 * @param agc Вказівник для збереження AGC (0-255)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadAGC(AS5600_Driver_t* driver, uint8_t* agc);

/**
 * @brief Читання magnitude (сила магнітного поля)
 *
 * @param driver Вказівник на структуру драйвера
 * @param magnitude Вказівник для збереження magnitude
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_ReadMagnitude(AS5600_Driver_t* driver, uint16_t* magnitude);

/**
 * @brief Встановлення нульової позиції (ZPOS)
 *
 * Встановлює регістр ZPOS для визначення нульової точки.
 *
 * @param driver Вказівник на структуру драйвера
 * @param position Позиція (0-4095)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_SetZeroPosition(AS5600_Driver_t* driver, uint16_t position);

/**
 * @brief Встановлення максимальної позиції (MPOS)
 *
 * Встановлює регістр MPOS для визначення максимальної позиції обертів.
 *
 * @param driver Вказівник на структуру драйвера
 * @param position Позиція (0-4095)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_SetMaxPosition(AS5600_Driver_t* driver, uint16_t position);

/**
 * @brief Програмування ZPOS/MPOS в OTP пам'ять
 *
 * УВАГА: Можна записати тільки 3 рази! Перевірте ZMCO регістр перед викликом.
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AS5600_BurnSettings(AS5600_Driver_t* driver);

/* Адреси регістрів AS5600 --------------------------------------------------*/

#define AS5600_I2C_ADDRESS      0x36  /**< 7-bit I2C адреса */

#define AS5600_REG_ZMCO         0x00  /**< Кількість записів ZPOS/MPOS (0-3) */
#define AS5600_REG_ZPOS_H       0x01  /**< Zero position high byte */
#define AS5600_REG_ZPOS_L       0x02  /**< Zero position low byte */
#define AS5600_REG_MPOS_H       0x03  /**< Maximum position high byte */
#define AS5600_REG_MPOS_L       0x04  /**< Maximum position low byte */
#define AS5600_REG_MANG_H       0x05  /**< Maximum angle high byte */
#define AS5600_REG_MANG_L       0x06  /**< Maximum angle low byte */
#define AS5600_REG_CONF_H       0x07  /**< Configuration high byte */
#define AS5600_REG_CONF_L       0x08  /**< Configuration low byte */
#define AS5600_REG_RAW_ANGLE_H  0x0C  /**< Raw angle high byte (без ZPOS/MPOS) */
#define AS5600_REG_RAW_ANGLE_L  0x0D  /**< Raw angle low byte */
#define AS5600_REG_ANGLE_H      0x0E  /**< Angle high byte (з ZPOS/MPOS) */
#define AS5600_REG_ANGLE_L      0x0F  /**< Angle low byte */
#define AS5600_REG_STATUS       0x0B  /**< Status register */
#define AS5600_REG_AGC          0x1A  /**< Automatic Gain Control */
#define AS5600_REG_MAGNITUDE_H  0x1B  /**< Magnitude high byte */
#define AS5600_REG_MAGNITUDE_L  0x1C  /**< Magnitude low byte */
#define AS5600_REG_BURN         0xFF  /**< Burn settings command */

/* Біти статусу -------------------------------------------------------------*/

#define AS5600_STATUS_MH        (1 << 3)  /**< Magnet too strong */
#define AS5600_STATUS_ML        (1 << 4)  /**< Magnet too weak */
#define AS5600_STATUS_MD        (1 << 5)  /**< Magnet detected */

/* Константи ----------------------------------------------------------------*/

#define AS5600_MAX_VALUE        4095      /**< Максимальне 12-bit значення */
#define AS5600_BURN_ANGLE       0x80      /**< Burn angle command */
#define AS5600_BURN_SETTING     0x40      /**< Burn setting command */

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_AS5600_H */
