/**
 * @file as5600.c
 * @brief Реалізація драйвера AS5600
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_AS5600

/* Auto-enable dependencies */

#ifndef USE_SENSOR_I2C_HELPER
 #define USE_SENSOR_I2C_HELPER
#endif
#ifndef USE_SENSOR_POSITION
 #define USE_SENSOR_POSITION
#endif

#include "../../../Inc/drv/sensor/as5600.h"
#include "../../../Inc/ctrl/time.h"
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t AS5600_Init_Internal(Sensor_Interface_t* self,
                                            const Sensor_Params_t* params);
static Servo_Status_t AS5600_DeInit_Internal(Sensor_Interface_t* self);
static Servo_Status_t AS5600_ReadAngle_Internal(Sensor_Interface_t* self, float* angle);
static Servo_Status_t AS5600_ReadVelocity_Internal(Sensor_Interface_t* self, float* velocity);
static Servo_Status_t AS5600_Calibrate_Internal(Sensor_Interface_t* self);
static Servo_Status_t AS5600_SelfTest_Internal(Sensor_Interface_t* self);
static Servo_Status_t AS5600_GetState_Internal(Sensor_Interface_t* self, Sensor_State_t* state);
static Servo_Status_t AS5600_GetStats_Internal(Sensor_Interface_t* self, Sensor_Stats_t* stats);

/* Private functions ---------------------------------------------------------*/

static inline AS5600_Driver_t* GetDriver(Sensor_Interface_t* self)
{
    return (AS5600_Driver_t*)self->driver_data;
}

/**
 * @brief Читання 16-bit регістру
 */
static Servo_Status_t AS5600_ReadReg16(AS5600_Driver_t* driver, uint8_t reg, uint16_t* value)
{
    uint8_t data[2];
    Servo_Status_t status;

    status = HWD_I2C_ReadReg(driver->config.i2c, driver->config.address << 1, reg, data, 2);
    if (status != SERVO_OK) {
        return status;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];
    return SERVO_OK;
}

/**
 * @brief Запис 16-bit регістру
 */
static Servo_Status_t AS5600_WriteReg16(AS5600_Driver_t* driver, uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;

    return HWD_I2C_WriteReg(driver->config.i2c, driver->config.address << 1, reg, data, 2);
}

/**
 * @brief Обчислення швидкості на основі зміни кута
 */
static void AS5600_CalculateVelocity(AS5600_Driver_t* driver)
{
    uint32_t current_time = Time_GetMillis();
    uint32_t dt = current_time - driver->data.last_read_time;

    if (dt == 0 || driver->data.last_read_time == 0) {
        driver->data.velocity = 0.0f;
        driver->data.last_read_time = current_time;
        driver->data.last_raw_angle = driver->data.raw_angle;
        return;
    }

    int32_t angle_diff = (int32_t)driver->data.raw_angle - (int32_t)driver->data.last_raw_angle;

    // Обробка переходу через нуль
    if (angle_diff > AS5600_MAX_VALUE / 2) {
        angle_diff -= AS5600_MAX_VALUE;
        driver->data.revolution_count--;
    } else if (angle_diff < -AS5600_MAX_VALUE / 2) {
        angle_diff += AS5600_MAX_VALUE;
        driver->data.revolution_count++;
    }

    // Швидкість в градусах/секунду
    float angle_change = (float)angle_diff * AS5600_RESOLUTION;
    float dt_sec = (float)dt / 1000.0f;
    driver->data.velocity = angle_change / dt_sec;

    driver->data.last_read_time = current_time;
    driver->data.last_raw_angle = driver->data.raw_angle;
}

/* Interface implementation --------------------------------------------------*/

static Servo_Status_t AS5600_Init_Internal(Sensor_Interface_t* self,
                                            const Sensor_Params_t* params)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка доступності пристрою
    Servo_Status_t status = HWD_I2C_IsDeviceReady(driver->config.i2c,
                                                   driver->config.address << 1,
                                                   3);
    if (status != SERVO_OK) {
        return SERVO_ERROR;
    }

    // Початкове читання
    status = AS5600_Update(driver);
    if (status != SERVO_OK) {
        return status;
    }

    // Перевірка магніту
    if (driver->data.magnet == AS5600_MAGNET_NOT_DETECTED) {
        return SERVO_ERROR;
    }

    driver->is_initialized = true;
    driver->stats.state = SENSOR_STATE_READY;

    return SERVO_OK;
}

static Servo_Status_t AS5600_DeInit_Internal(Sensor_Interface_t* self)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    driver->is_initialized = false;
    driver->stats.state = SENSOR_STATE_IDLE;

    return SERVO_OK;
}

static Servo_Status_t AS5600_ReadAngle_Internal(Sensor_Interface_t* self, float* angle)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL || angle == NULL) {
        return SERVO_INVALID;
    }

    Servo_Status_t status = AS5600_Update(driver);
    if (status != SERVO_OK) {
        return status;
    }

    *angle = driver->data.angle_degrees;
    return SERVO_OK;
}

static Servo_Status_t AS5600_ReadVelocity_Internal(Sensor_Interface_t* self, float* velocity)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL || velocity == NULL) {
        return SERVO_INVALID;
    }

    *velocity = driver->data.velocity;
    return SERVO_OK;
}

static Servo_Status_t AS5600_Calibrate_Internal(Sensor_Interface_t* self)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Встановлення поточного положення як нульового
    return AS5600_SetZeroPosition(driver, driver->data.raw_angle);
}

static Servo_Status_t AS5600_SelfTest_Internal(Sensor_Interface_t* self)
{
    return AS5600_SelfTest(GetDriver(self));
}

static Servo_Status_t AS5600_GetState_Internal(Sensor_Interface_t* self, Sensor_State_t* state)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL || state == NULL) {
        return SERVO_INVALID;
    }

    *state = driver->stats.state;
    return SERVO_OK;
}

static Servo_Status_t AS5600_GetStats_Internal(Sensor_Interface_t* self, Sensor_Stats_t* stats)
{
    AS5600_Driver_t* driver = GetDriver(self);
    if (driver == NULL || stats == NULL) {
        return SERVO_INVALID;
    }

    *stats = driver->stats;
    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t AS5600_Create(AS5600_Driver_t* driver, const AS5600_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    memset(driver, 0, sizeof(AS5600_Driver_t));

    driver->config = *config;

    // Налаштування інтерфейсу
    driver->interface.init = AS5600_Init_Internal;
    driver->interface.deinit = AS5600_DeInit_Internal;
    driver->interface.read_angle = AS5600_ReadAngle_Internal;
    driver->interface.read_velocity = AS5600_ReadVelocity_Internal;
    driver->interface.calibrate = AS5600_Calibrate_Internal;
    driver->interface.self_test = AS5600_SelfTest_Internal;
    driver->interface.get_state = AS5600_GetState_Internal;
    driver->interface.get_stats = AS5600_GetStats_Internal;
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

Servo_Status_t AS5600_Init(AS5600_Driver_t* driver, const Sensor_Params_t* params)
{
    return AS5600_Init_Internal(&driver->interface, params);
}

Servo_Status_t AS5600_DeInit(AS5600_Driver_t* driver)
{
    return AS5600_DeInit_Internal(&driver->interface);
}

Servo_Status_t AS5600_ReadRawAngle(AS5600_Driver_t* driver, uint16_t* raw_angle)
{
    if (driver == NULL || !driver->is_initialized || raw_angle == NULL) {
        return SERVO_INVALID;
    }

    uint8_t reg = driver->config.use_raw_angle ? AS5600_REG_RAW_ANGLE_H : AS5600_REG_ANGLE_H;

    return AS5600_ReadReg16(driver, reg, raw_angle);
}

Servo_Status_t AS5600_ReadAngle(AS5600_Driver_t* driver, float* angle)
{
    if (driver == NULL || !driver->is_initialized || angle == NULL) {
        return SERVO_INVALID;
    }

    uint16_t raw_angle;
    Servo_Status_t status = AS5600_ReadRawAngle(driver, &raw_angle);
    if (status != SERVO_OK) {
        return status;
    }

    *angle = (float)raw_angle * AS5600_RESOLUTION;
    return SERVO_OK;
}

Servo_Status_t AS5600_ReadVelocity(AS5600_Driver_t* driver, float* velocity)
{
    if (driver == NULL || !driver->is_initialized || velocity == NULL) {
        return SERVO_INVALID;
    }

    *velocity = driver->data.velocity;
    return SERVO_OK;
}

Servo_Status_t AS5600_Update(AS5600_Driver_t* driver)
{
    if (driver == NULL || !driver->is_initialized) {
        return SERVO_INVALID;
    }

    driver->stats.state = SENSOR_STATE_READING;

    // Читання кута
    Servo_Status_t status = AS5600_ReadRawAngle(driver, &driver->data.raw_angle);
    if (status != SERVO_OK) {
        driver->stats.error_count++;
        driver->stats.state = SENSOR_STATE_ERROR;
        return status;
    }

    // Конвертація в градуси
    driver->data.angle_degrees = (float)driver->data.raw_angle * AS5600_RESOLUTION;

    // Обчислення швидкості
    AS5600_CalculateVelocity(driver);

    // Читання статусу
    uint8_t status_reg;
    status = HWD_I2C_ReadRegByte(driver->config.i2c,
                                 driver->config.address << 1,
                                 AS5600_REG_STATUS,
                                 &status_reg);

    if (status == SERVO_OK) {
        // Визначення стану магніту
        if (!(status_reg & AS5600_STATUS_MD)) {
            driver->data.magnet = AS5600_MAGNET_NOT_DETECTED;
        } else if (status_reg & AS5600_STATUS_ML) {
            driver->data.magnet = AS5600_MAGNET_TOO_WEAK;
        } else if (status_reg & AS5600_STATUS_MH) {
            driver->data.magnet = AS5600_MAGNET_TOO_STRONG;
        } else {
            driver->data.magnet = AS5600_MAGNET_GOOD;
        }
    }

    // Оновлення статистики
    driver->stats.read_count++;
    driver->stats.last_angle = driver->data.angle_degrees;
    driver->stats.last_velocity = driver->data.velocity;
    driver->stats.state = SENSOR_STATE_READY;

    return SERVO_OK;
}

Servo_Status_t AS5600_GetMagnetStatus(AS5600_Driver_t* driver,
                                      AS5600_MagnetStatus_t* status)
{
    if (driver == NULL || !driver->is_initialized || status == NULL) {
        return SERVO_INVALID;
    }

    *status = driver->data.magnet;
    return SERVO_OK;
}

Servo_Status_t AS5600_ReadAGC(AS5600_Driver_t* driver, uint8_t* agc)
{
    if (driver == NULL || !driver->is_initialized || agc == NULL) {
        return SERVO_INVALID;
    }

    return HWD_I2C_ReadRegByte(driver->config.i2c,
                               driver->config.address << 1,
                               AS5600_REG_AGC,
                               agc);
}

Servo_Status_t AS5600_ReadMagnitude(AS5600_Driver_t* driver, uint16_t* magnitude)
{
    if (driver == NULL || !driver->is_initialized || magnitude == NULL) {
        return SERVO_INVALID;
    }

    return AS5600_ReadReg16(driver, AS5600_REG_MAGNITUDE_H, magnitude);
}

Servo_Status_t AS5600_SetZeroPosition(AS5600_Driver_t* driver, uint16_t position)
{
    if (driver == NULL || !driver->is_initialized) {
        return SERVO_INVALID;
    }

    if (position > AS5600_MAX_VALUE) {
        return SERVO_INVALID;
    }

    return AS5600_WriteReg16(driver, AS5600_REG_ZPOS_H, position);
}

Servo_Status_t AS5600_SetMaxPosition(AS5600_Driver_t* driver, uint16_t position)
{
    if (driver == NULL || !driver->is_initialized) {
        return SERVO_INVALID;
    }

    if (position > AS5600_MAX_VALUE) {
        return SERVO_INVALID;
    }

    return AS5600_WriteReg16(driver, AS5600_REG_MPOS_H, position);
}

Sensor_Interface_t* AS5600_GetInterface(AS5600_Driver_t* driver)
{
    if (driver == NULL) {
        return NULL;
    }
    return &driver->interface;
}

Servo_Status_t AS5600_SelfTest(AS5600_Driver_t* driver)
{
    if (driver == NULL || !driver->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка зв'язку
    Servo_Status_t status = HWD_I2C_IsDeviceReady(driver->config.i2c,
                                                   driver->config.address << 1,
                                                   3);
    if (status != SERVO_OK) {
        return SERVO_ERROR;
    }

    // Перевірка магніту
    status = AS5600_Update(driver);
    if (status != SERVO_OK) {
        return status;
    }

    if (driver->data.magnet != AS5600_MAGNET_GOOD) {
        return SERVO_ERROR;
    }

    return SERVO_OK;
}

#endif /* USE_SENSOR_AS5600 */
