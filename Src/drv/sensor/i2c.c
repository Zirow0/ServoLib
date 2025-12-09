/**
 * @file i2c.c
 * @brief Реалізація допоміжних I2C функцій для датчиків
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Auto-enable dependencies */
#ifndef USE_HWD_I2C
	#define USE_HWD_I2C
#endif

#ifdef USE_SENSOR_I2C_HELPER

#include "../../../Inc/drv/sensor/i2c.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t I2C_Sensor_Init(I2C_Sensor_Handle_t* handle,
                               const I2C_Sensor_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->i2c_handle == NULL) {
        return SERVO_INVALID;
    }

    // Копіювання конфігурації
    handle->config = *config;

    // Встановлення значень за замовчуванням
    if (handle->config.max_retries == 0) {
        handle->config.max_retries = I2C_SENSOR_DEFAULT_RETRIES;
    }

    if (handle->config.retry_delay_ms == 0) {
        handle->config.retry_delay_ms = I2C_SENSOR_RETRY_DELAY_MS;
    }

    // Скидання лічильників
    handle->error_count = 0;
    handle->success_count = 0;
    handle->last_error = SERVO_OK;

    return SERVO_OK;
}

Servo_Status_t I2C_Sensor_IsReady(I2C_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    Servo_Status_t status;
    uint8_t retries = handle->config.max_retries;

    while (retries > 0) {
        status = HWD_I2C_IsDeviceReady(
            handle->config.i2c_handle,
            handle->config.device_address,
            1
        );

        if (status == SERVO_OK) {
            handle->success_count++;
            handle->last_error = SERVO_OK;
            return SERVO_OK;
        }

        retries--;
        if (retries > 0) {
            HWD_Timer_DelayMs(handle->config.retry_delay_ms);
        }
    }

    handle->error_count++;
    handle->last_error = status;
    return status;
}

Servo_Status_t I2C_Sensor_ReadReg(I2C_Sensor_Handle_t* handle,
                                  uint8_t reg_address,
                                  uint8_t* data,
                                  uint16_t size)
{
    if (handle == NULL || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    Servo_Status_t status;
    uint8_t retries = handle->config.max_retries;

    while (retries > 0) {
        status = HWD_I2C_ReadReg(
            handle->config.i2c_handle,
            handle->config.device_address,
            reg_address,
            data,
            size
        );

        if (status == SERVO_OK) {
            handle->success_count++;
            handle->last_error = SERVO_OK;
            return SERVO_OK;
        }

        retries--;
        if (retries > 0) {
            HWD_Timer_DelayMs(handle->config.retry_delay_ms);
        }
    }

    handle->error_count++;
    handle->last_error = status;
    return status;
}

Servo_Status_t I2C_Sensor_WriteReg(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   const uint8_t* data,
                                   uint16_t size)
{
    if (handle == NULL || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    Servo_Status_t status;
    uint8_t retries = handle->config.max_retries;

    while (retries > 0) {
        status = HWD_I2C_WriteReg(
            handle->config.i2c_handle,
            handle->config.device_address,
            reg_address,
            data,
            size
        );

        if (status == SERVO_OK) {
            handle->success_count++;
            handle->last_error = SERVO_OK;
            return SERVO_OK;
        }

        retries--;
        if (retries > 0) {
            HWD_Timer_DelayMs(handle->config.retry_delay_ms);
        }
    }

    handle->error_count++;
    handle->last_error = status;
    return status;
}

Servo_Status_t I2C_Sensor_ReadByte(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   uint8_t* value)
{
    if (value == NULL) {
        return SERVO_INVALID;
    }

    return I2C_Sensor_ReadReg(handle, reg_address, value, 1);
}

Servo_Status_t I2C_Sensor_WriteByte(I2C_Sensor_Handle_t* handle,
                                    uint8_t reg_address,
                                    uint8_t value)
{
    return I2C_Sensor_WriteReg(handle, reg_address, &value, 1);
}

Servo_Status_t I2C_Sensor_ReadWord16BE(I2C_Sensor_Handle_t* handle,
                                       uint8_t reg_address,
                                       uint16_t* value)
{
    if (value == NULL) {
        return SERVO_INVALID;
    }

    uint8_t data[2];
    Servo_Status_t status = I2C_Sensor_ReadReg(handle, reg_address, data, 2);

    if (status == SERVO_OK) {
        // Big Endian: старший байт першим
        *value = ((uint16_t)data[0] << 8) | data[1];
    }

    return status;
}

Servo_Status_t I2C_Sensor_ReadWord16LE(I2C_Sensor_Handle_t* handle,
                                       uint8_t reg_address,
                                       uint16_t* value)
{
    if (value == NULL) {
        return SERVO_INVALID;
    }

    uint8_t data[2];
    Servo_Status_t status = I2C_Sensor_ReadReg(handle, reg_address, data, 2);

    if (status == SERVO_OK) {
        // Little Endian: молодший байт першим
        *value = ((uint16_t)data[1] << 8) | data[0];
    }

    return status;
}

Servo_Status_t I2C_Sensor_WriteBit(I2C_Sensor_Handle_t* handle,
                                   uint8_t reg_address,
                                   uint8_t bit_num,
                                   uint8_t value)
{
    if (bit_num > 7) {
        return SERVO_INVALID;
    }

    // Читання поточного значення
    uint8_t reg_value;
    Servo_Status_t status = I2C_Sensor_ReadByte(handle, reg_address, &reg_value);

    if (status != SERVO_OK) {
        return status;
    }

    // Зміна біта
    if (value) {
        reg_value |= (1 << bit_num);   // Встановити біт
    } else {
        reg_value &= ~(1 << bit_num);  // Скинути біт
    }

    // Запис нового значення
    return I2C_Sensor_WriteByte(handle, reg_address, reg_value);
}

Servo_Status_t I2C_Sensor_ReadBit(I2C_Sensor_Handle_t* handle,
                                  uint8_t reg_address,
                                  uint8_t bit_num,
                                  uint8_t* value)
{
    if (bit_num > 7 || value == NULL) {
        return SERVO_INVALID;
    }

    // Читання регістра
    uint8_t reg_value;
    Servo_Status_t status = I2C_Sensor_ReadByte(handle, reg_address, &reg_value);

    if (status == SERVO_OK) {
        *value = (reg_value >> bit_num) & 0x01;
    }

    return status;
}

Servo_Status_t I2C_Sensor_GetStats(const I2C_Sensor_Handle_t* handle,
                                   uint32_t* error_count,
                                   uint32_t* success_count)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (error_count != NULL) {
        *error_count = handle->error_count;
    }

    if (success_count != NULL) {
        *success_count = handle->success_count;
    }

    return SERVO_OK;
}

Servo_Status_t I2C_Sensor_ResetStats(I2C_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    handle->error_count = 0;
    handle->success_count = 0;
    handle->last_error = SERVO_OK;

    return SERVO_OK;
}

#endif /* USE_SENSOR_I2C_HELPER */
