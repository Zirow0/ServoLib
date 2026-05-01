/**
 * @file as5600.h
 * @brief Драйвер магнітного енкодера AS5600 (I2C, 12-bit)
 *
 * Читання кута через I2C IT continuous mode: дані постійно оновлюються
 * в volatile raw_buf[2] фоновим I2C IRQ. read_raw() читає буфер миттєво.
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

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан магніту AS5600
 */
typedef enum {
    AS5600_MAGNET_NOT_DETECTED = 0,
    AS5600_MAGNET_TOO_WEAK     = 1,
    AS5600_MAGNET_GOOD         = 2,
    AS5600_MAGNET_TOO_STRONG   = 3
} AS5600_MagnetStatus_t;

/**
 * @brief Конфігурація AS5600
 */
typedef struct {
    HWD_I2C_Config_t i2c_config;
} AS5600_Config_t;

/**
 * @brief Структура драйвера AS5600
 *
 * Перше поле — Position_Sensor_Interface_t (обов'язково).
 */
typedef struct {
    Position_Sensor_Interface_t interface;   /**< Інтерфейс датчика (ПЕРШИЙ!) */
    AS5600_Config_t             config;
    HWD_I2C_Handle_t            i2c_handle;

    volatile uint8_t            raw_buf[2];       /**< Оновлюється I2C IT IRQ */
    AS5600_MagnetStatus_t       magnet_status;
    uint32_t                    error_count;
    uint16_t                    last_raw;         /**< Попередній 12-bit raw для детекції обертів */
    int32_t                     revolution_count; /**< Лічильник обертів */
} AS5600_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера AS5600
 *
 * Прив'язує callbacks до interface. Після виклику:
 * Position_Sensor_Init(&driver->interface).
 */
Servo_Status_t AS5600_Create(AS5600_Driver_t* driver,
                              const AS5600_Config_t* config);

/**
 * @brief Зчитування стану магніту (для діагностики)
 *
 * Виконує блокуючий I2C read регістру STATUS.
 * Викликати після Init, до початку control loop.
 */
Servo_Status_t AS5600_ReadMagnetStatus(AS5600_Driver_t* driver);

/* Адреси регістрів AS5600 --------------------------------------------------*/

#define AS5600_I2C_ADDRESS      0x36

#define AS5600_REG_ANGLE_H      0x0E
#define AS5600_REG_ANGLE_L      0x0F
#define AS5600_REG_STATUS       0x0B

#define AS5600_STATUS_MH        (1 << 3)
#define AS5600_STATUS_ML        (1 << 4)
#define AS5600_STATUS_MD        (1 << 5)

#define AS5600_MAX_VALUE        4095

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_AS5600_H */
