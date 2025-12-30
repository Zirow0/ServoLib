/**
 * @file as5600.c
 * @brief Реалізація драйвера магнітного енкодера AS5600
 * @author ServoCore Team
 * @date 2025
 *
 * Hardware Callbacks Pattern: тільки апаратні операції (I2C read/write).
 * Вся логіка (конвертація raw→degrees, velocity, multi-turn) в position.c.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_AS5600

/* Auto-enable dependencies */
#ifndef USE_SENSOR_POSITION
	#define USE_SENSOR_POSITION
#endif

#include "drv/position/as5600.h"
#include "hwd/hwd_timer.h"
#include <string.h>

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief Читання 16-bit регістру через I2C
 */
static Servo_Status_t AS5600_ReadReg16(AS5600_Driver_t* driver, uint8_t reg, uint16_t* value)
{
    uint8_t data[2];
    Servo_Status_t status;

    status = HWD_I2C_ReadReg(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1, reg, data, 2);
    if (status != SERVO_OK) {
        driver->error_count++;
        return status;
    }

    // AS5600 передає дані в big-endian форматі
    *value = ((uint16_t)data[0] << 8) | data[1];
    return SERVO_OK;
}

/**
 * @brief Запис 16-bit регістру через I2C
 */
static Servo_Status_t AS5600_WriteReg16(AS5600_Driver_t* driver, uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;

    Servo_Status_t status = HWD_I2C_WriteReg(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1,
                                              reg, data, 2);
    if (status != SERVO_OK) {
        driver->error_count++;
    }
    return status;
}

/**
 * @brief Читання 8-bit регістру через I2C
 */
static Servo_Status_t AS5600_ReadReg8(AS5600_Driver_t* driver, uint8_t reg, uint8_t* value)
{
    Servo_Status_t status = HWD_I2C_ReadReg(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1,
                                             reg, value, 1);
    if (status != SERVO_OK) {
        driver->error_count++;
    }
    return status;
}

/**
 * @brief Запис 8-bit регістру через I2C
 */
static Servo_Status_t AS5600_WriteReg8(AS5600_Driver_t* driver, uint8_t reg, uint8_t value)
{
    Servo_Status_t status = HWD_I2C_WriteReg(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1,
                                              reg, &value, 1);
    if (status != SERVO_OK) {
        driver->error_count++;
    }
    return status;
}

/* Private hardware callbacks ------------------------------------------------*/

/**
 * @brief Hardware Init Callback
 */
static Servo_Status_t AS5600_HW_Init(void* driver_data, const Position_Params_t* params)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;
    Servo_Status_t status;

    // Ініціалізація I2C
    status = HWD_I2C_Init(&driver->i2c_handle, &driver->config.i2c_config);
    if (status != SERVO_OK) {
        return status;
    }

    // Перевірка доступності пристрою
    status = HWD_I2C_IsDeviceReady(&driver->i2c_handle, AS5600_I2C_ADDRESS << 1, 3);
    if (status != SERVO_OK) {
        return SERVO_ERROR;
    }

    // Зчитати та перевірити статус магніту
    status = AS5600_ReadMagnetStatus(driver);
    if (status != SERVO_OK) {
        return status;
    }

    if (driver->magnet_status == AS5600_MAGNET_NOT_DETECTED) {
        return SERVO_ERROR;  // Магніт не виявлено!
    }

    return SERVO_OK;
}

/**
 * @brief Hardware DeInit Callback
 */
static Servo_Status_t AS5600_HW_DeInit(void* driver_data)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;

    // Деініціалізувати I2C
    return HWD_I2C_DeInit(&driver->i2c_handle);
}

/**
 * @brief Hardware Read Raw Callback (КЛЮЧОВА ФУНКЦІЯ!)
 *
 * Читає ТІЛЬКИ сирі дані через I2C, БЕЗ конвертації в градуси.
 * Конвертацію робить position.c.
 */
static Servo_Status_t AS5600_HW_ReadRaw(void* driver_data, Position_Raw_Data_t* raw)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;
    Servo_Status_t status;
    uint16_t angle_raw;

    // Вибір регістру в залежності від налаштувань
    uint8_t reg = driver->config.use_raw_angle ? AS5600_REG_RAW_ANGLE_H : AS5600_REG_ANGLE_H;

    // Читати raw position (0-4095)
    status = AS5600_ReadReg16(driver, reg, &angle_raw);
    if (status != SERVO_OK) {
        raw->valid = false;
        return status;
    }

    // Заповнити структуру RAW даних
    raw->raw_position = angle_raw;
    raw->timestamp_us = HWD_Timer_GetMicros();
    raw->has_velocity = false;  // AS5600 не надає готову velocity
    raw->raw_velocity = 0.0f;
    raw->valid = true;

    return SERVO_OK;
}

/**
 * @brief Hardware Calibrate Callback
 *
 * Встановлює поточну позицію як нульову (ZPOS).
 */
static Servo_Status_t AS5600_HW_Calibrate(void* driver_data)
{
    AS5600_Driver_t* driver = (AS5600_Driver_t*)driver_data;

    // Зчитати поточну позицію
    Position_Raw_Data_t raw;
    Servo_Status_t status = AS5600_HW_ReadRaw(driver_data, &raw);
    if (status != SERVO_OK || !raw.valid) {
        return SERVO_ERROR;
    }

    // Встановити як ZPOS
    return AS5600_SetZeroPosition(driver, (uint16_t)raw.raw_position);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t AS5600_Create(AS5600_Driver_t* driver,
                              const AS5600_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    // Очистити структуру
    memset(driver, 0, sizeof(AS5600_Driver_t));

    // Копіювати конфігурацію
    driver->config = *config;

    // Налаштувати hardware callbacks
    driver->interface.hw.init = AS5600_HW_Init;
    driver->interface.hw.deinit = AS5600_HW_DeInit;
    driver->interface.hw.read_raw = AS5600_HW_ReadRaw;
    driver->interface.hw.calibrate = AS5600_HW_Calibrate;
    driver->interface.hw.notify_callback = NULL;  // I2C не підтримує interrupt-driven mode

    // Встановити можливості
    driver->interface.capabilities = POSITION_CAP_ABSOLUTE;  // AS5600 = абсолютний датчик

    // Встановити роздільну здатність
    driver->interface.resolution_bits = 12;  // 12-bit = 4096 позицій

    // Прив'язати driver_data
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

Servo_Status_t AS5600_ReadMagnetStatus(AS5600_Driver_t* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    uint8_t status_reg;
    Servo_Status_t status = AS5600_ReadReg8(driver, AS5600_REG_STATUS, &status_reg);
    if (status != SERVO_OK) {
        return status;
    }

    // Визначення стану магніту на основі битів статусу
    if (!(status_reg & AS5600_STATUS_MD)) {
        driver->magnet_status = AS5600_MAGNET_NOT_DETECTED;
    } else if (status_reg & AS5600_STATUS_ML) {
        driver->magnet_status = AS5600_MAGNET_TOO_WEAK;
    } else if (status_reg & AS5600_STATUS_MH) {
        driver->magnet_status = AS5600_MAGNET_TOO_STRONG;
    } else {
        driver->magnet_status = AS5600_MAGNET_GOOD;
    }

    // Зчитати AGC та magnitude для діагностики
    AS5600_ReadAGC(driver, &driver->agc);
    AS5600_ReadMagnitude(driver, &driver->magnitude);

    return SERVO_OK;
}

Servo_Status_t AS5600_ReadAGC(AS5600_Driver_t* driver, uint8_t* agc)
{
    if (driver == NULL || agc == NULL) {
        return SERVO_INVALID;
    }

    return AS5600_ReadReg8(driver, AS5600_REG_AGC, agc);
}

Servo_Status_t AS5600_ReadMagnitude(AS5600_Driver_t* driver, uint16_t* magnitude)
{
    if (driver == NULL || magnitude == NULL) {
        return SERVO_INVALID;
    }

    return AS5600_ReadReg16(driver, AS5600_REG_MAGNITUDE_H, magnitude);
}

Servo_Status_t AS5600_SetZeroPosition(AS5600_Driver_t* driver, uint16_t position)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    if (position > AS5600_MAX_VALUE) {
        return SERVO_INVALID;
    }

    return AS5600_WriteReg16(driver, AS5600_REG_ZPOS_H, position);
}

Servo_Status_t AS5600_SetMaxPosition(AS5600_Driver_t* driver, uint16_t position)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    if (position > AS5600_MAX_VALUE) {
        return SERVO_INVALID;
    }

    return AS5600_WriteReg16(driver, AS5600_REG_MPOS_H, position);
}

Servo_Status_t AS5600_BurnSettings(AS5600_Driver_t* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Перевірити ZMCO (кількість записів)
    uint8_t zmco;
    Servo_Status_t status = AS5600_ReadReg8(driver, AS5600_REG_ZMCO, &zmco);
    if (status != SERVO_OK) {
        return status;
    }

    if (zmco >= 3) {
        return SERVO_ERROR;  // Перевищено ліміт записів!
    }

    // Burn command
    status = AS5600_WriteReg8(driver, AS5600_REG_BURN, AS5600_BURN_SETTING);
    if (status != SERVO_OK) {
        return status;
    }

    // Зачекати ~10мс для запису в EEPROM
    HAL_Delay(10);

    return SERVO_OK;
}

#endif /* USE_SENSOR_AS5600 */
