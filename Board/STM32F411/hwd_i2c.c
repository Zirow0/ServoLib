/**
 * @file hwd_i2c.c
 * @brief Реалізація HWD I2C для STM32F411
 * @author ServoCore Team
 * @date 2025
 *
 * Платформо-специфічна реалізація I2C абстракції для STM32F4xx.
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"

#ifdef USE_HWD_I2C

#include "hwd/hwd_i2c.h"

/* Private defines -----------------------------------------------------------*/

/** @brief Таймаут за замовчуванням (мс) */
#define DEFAULT_TIMEOUT_MS      100

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Конвертація HAL статусу в Servo статус
 */
static inline Servo_Status_t ConvertHALStatus(HAL_StatusTypeDef hal_status)
{
    switch (hal_status) {
        case HAL_OK:
            return SERVO_OK;
        case HAL_TIMEOUT:
            return SERVO_TIMEOUT;
        case HAL_BUSY:
            return SERVO_BUSY;
        default:
            return SERVO_ERROR;
    }
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_I2C_Init(HWD_I2C_Handle_t* handle, const HWD_I2C_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->hw_handle == NULL) {
        return SERVO_INVALID;
    }

    // Копіювання конфігурації
    handle->config = *config;

    // Встановлення таймауту за замовчуванням якщо не вказано
    if (handle->config.timeout_ms == 0) {
        handle->config.timeout_ms = DEFAULT_TIMEOUT_MS;
    }

    // STM32 HAL I2C вже ініціалізовано через CubeMX
    // Просто зберігаємо конфігурацію
    handle->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_DeInit(HWD_I2C_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    handle->is_initialized = false;

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_Write(HWD_I2C_Handle_t* handle,
                             uint16_t dev_address,
                             const uint8_t* data,
                             uint16_t size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)handle->config.hw_handle;

    if (hi2c == NULL) {
        return SERVO_INVALID;
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(
        hi2c,
        dev_address,
        (uint8_t*)data,
        size,
        handle->config.timeout_ms
    );

    return ConvertHALStatus(hal_status);
}

Servo_Status_t HWD_I2C_Read(HWD_I2C_Handle_t* handle,
                            uint16_t dev_address,
                            uint8_t* data,
                            uint16_t size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)handle->config.hw_handle;

    if (hi2c == NULL) {
        return SERVO_INVALID;
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(
        hi2c,
        dev_address,
        data,
        size,
        handle->config.timeout_ms
    );

    return ConvertHALStatus(hal_status);
}

Servo_Status_t HWD_I2C_WriteReg(HWD_I2C_Handle_t* handle,
                                uint16_t dev_address,
                                uint8_t reg_address,
                                const uint8_t* data,
                                uint16_t size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)handle->config.hw_handle;

    if (hi2c == NULL) {
        return SERVO_INVALID;
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Write(
        hi2c,
        dev_address,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        (uint8_t*)data,
        size,
        handle->config.timeout_ms
    );

    return ConvertHALStatus(hal_status);
}

Servo_Status_t HWD_I2C_ReadReg(HWD_I2C_Handle_t* handle,
                               uint16_t dev_address,
                               uint8_t reg_address,
                               uint8_t* data,
                               uint16_t size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)handle->config.hw_handle;

    if (hi2c == NULL) {
        return SERVO_INVALID;
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Read(
        hi2c,
        dev_address,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        data,
        size,
        handle->config.timeout_ms
    );

    return ConvertHALStatus(hal_status);
}

Servo_Status_t HWD_I2C_WriteRegByte(HWD_I2C_Handle_t* handle,
                                    uint16_t dev_address,
                                    uint8_t reg_address,
                                    uint8_t value)
{
    return HWD_I2C_WriteReg(handle, dev_address, reg_address, &value, 1);
}

Servo_Status_t HWD_I2C_ReadRegByte(HWD_I2C_Handle_t* handle,
                                   uint16_t dev_address,
                                   uint8_t reg_address,
                                   uint8_t* value)
{
    if (value == NULL) {
        return SERVO_INVALID;
    }

    return HWD_I2C_ReadReg(handle, dev_address, reg_address, value, 1);
}

Servo_Status_t HWD_I2C_IsDeviceReady(HWD_I2C_Handle_t* handle,
                                     uint16_t dev_address,
                                     uint8_t trials)
{
    if (handle == NULL || !handle->is_initialized) {
        return SERVO_INVALID;
    }

    I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)handle->config.hw_handle;

    if (hi2c == NULL) {
        return SERVO_INVALID;
    }

    // Якщо trials = 0, встановлюємо 1 спробу
    if (trials == 0) {
        trials = 1;
    }

    HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(
        hi2c,
        dev_address,
        trials,
        handle->config.timeout_ms
    );

    return ConvertHALStatus(hal_status);
}

#endif /* USE_HWD_I2C */
