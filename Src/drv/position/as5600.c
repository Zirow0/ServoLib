/**
 * @file as5600.c
 * @brief Реалізація драйвера AS5600
 *
 * Init: ініціалізує I2C, перевіряє магніт, запускає фоновий continuous read.
 * read_raw: читає з volatile raw_buf — миттєво, без блокування.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_AS5600

#ifndef USE_SENSOR_POSITION
    #define USE_SENSOR_POSITION
#endif

#include "../../../Inc/drv/position/as5600.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define TWO_PI_F  6.28318530717958647f

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t ReadReg8(AS5600_Driver_t* driver, uint8_t reg, uint8_t* value)
{
    Servo_Status_t s = HWD_I2C_ReadRegByte(&driver->i2c_handle,
                                            AS5600_I2C_ADDRESS << 1,
                                            reg, value);
    if (s != SERVO_OK) driver->error_count++;
    return s;
}

/* Hardware callbacks --------------------------------------------------------*/

static Servo_Status_t AS5600_HW_Init(void* driver_data)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;

    Servo_Status_t s = HWD_I2C_Init(&driver->i2c_handle, &driver->config.i2c_config);
    if (s != SERVO_OK) return s;

    s = HWD_I2C_IsDeviceReady(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1, 3);
    if (s != SERVO_OK) return SERVO_ERROR;

    s = AS5600_ReadMagnetStatus(driver);
    if (s != SERVO_OK) return s;

    if (driver->magnet_status == AS5600_MAGNET_NOT_DETECTED) {
        return SERVO_ERROR;
    }

    /* Запуск фонового continuous read у raw_buf */
    return HWD_I2C_StartContinuousRead(&driver->i2c_handle,
                                        AS5600_I2C_ADDRESS << 1,
                                        AS5600_REG_ANGLE_H,
                                        driver->raw_buf, 2);
}

static Servo_Status_t AS5600_HW_ReadRaw(void* driver_data, Position_Raw_Data_t* raw)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;

    /* Читання з volatile буфера — оновлюється I2C IT IRQ */
    uint16_t angle_raw = (((uint16_t)driver->raw_buf[0] << 8)
                         | driver->raw_buf[1]) & AS5600_MAX_VALUE;

    raw->angle_rad      = (float)angle_raw * TWO_PI_F / 4096.0f;
    raw->timestamp_us   = HWD_Timer_GetMicros();
    raw->has_velocity   = false;
    raw->velocity_rad_s = 0.0f;
    raw->valid          = true;

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t AS5600_Create(AS5600_Driver_t* driver,
                              const AS5600_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    memset(driver, 0, sizeof(AS5600_Driver_t));

    driver->config = *config;

    driver->interface.hw.init     = AS5600_HW_Init;
    driver->interface.hw.read_raw = AS5600_HW_ReadRaw;
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

Servo_Status_t AS5600_ReadMagnetStatus(AS5600_Driver_t* driver)
{
    if (driver == NULL) return SERVO_INVALID;

    uint8_t status_reg;
    Servo_Status_t s = ReadReg8(driver, AS5600_REG_STATUS, &status_reg);
    if (s != SERVO_OK) return s;

    if (!(status_reg & AS5600_STATUS_MD)) {
        driver->magnet_status = AS5600_MAGNET_NOT_DETECTED;
    } else if (status_reg & AS5600_STATUS_ML) {
        driver->magnet_status = AS5600_MAGNET_TOO_WEAK;
    } else if (status_reg & AS5600_STATUS_MH) {
        driver->magnet_status = AS5600_MAGNET_TOO_STRONG;
    } else {
        driver->magnet_status = AS5600_MAGNET_GOOD;
    }

    return SERVO_OK;
}

#endif /* USE_SENSOR_AS5600 */
