/**
 * @file hwd_i2c.c
 * @brief Реалізація HWD I2C для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * I2C абстракція через libopencm3.
 *
 * Зберігання в HWD_I2C_Config_t:
 *   hw_handle → (void*)(uintptr_t)I2C1 — базова адреса I2C
 *
 * Адресація:
 *   Інтерфейс приймає 8-bit адресу (як HAL): dev_address = 7-bit addr << 1
 *   libopencm3 i2c_transfer7() очікує 7-bit адресу → ділимо на 2.
 *
 * Передумова: Board_Init() вже ініціалізував I2C та GPIO.
 *
 * Обмеження WriteReg:
 *   Тимчасовий буфер на стеку (регістр + дані). Не використовувати
 *   для size > 64 байт — розгляньте статичний буфер при потребі.
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"

#ifdef USE_HWD_I2C

#include "hwd/hwd_i2c.h"
#include <libopencm3/stm32/i2c.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define I2C_WRITE_BUF_MAX   65U   /**< Max bytes для WriteReg (1 reg + 64 data) */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Конвертація 8-bit HAL адреси в 7-bit для libopencm3
 */
static inline uint8_t to_7bit(uint16_t addr_8bit)
{
    return (uint8_t)(addr_8bit >> 1);
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

    handle->config = *config;

    /* I2C вже налаштований у Board_Init() — просто зберігаємо конфіг */
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
                              uint16_t          dev_address,
                              const uint8_t*    data,
                              uint16_t          size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    i2c_transfer7(i2c, to_7bit(dev_address), (uint8_t*)data, size, NULL, 0);

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_Read(HWD_I2C_Handle_t* handle,
                             uint16_t          dev_address,
                             uint8_t*          data,
                             uint16_t          size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    i2c_transfer7(i2c, to_7bit(dev_address), NULL, 0, data, size);

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_WriteReg(HWD_I2C_Handle_t* handle,
                                 uint16_t          dev_address,
                                 uint8_t           reg_address,
                                 const uint8_t*    data,
                                 uint16_t          size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    if ((size + 1U) > I2C_WRITE_BUF_MAX) {
        return SERVO_INVALID;
    }

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    /* Об'єднуємо адресу регістра і дані в один буфер на стеку */
    uint8_t buf[I2C_WRITE_BUF_MAX];
    buf[0] = reg_address;
    memcpy(&buf[1], data, size);

    i2c_transfer7(i2c, to_7bit(dev_address), buf, size + 1U, NULL, 0);

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_ReadReg(HWD_I2C_Handle_t* handle,
                                uint16_t          dev_address,
                                uint8_t           reg_address,
                                uint8_t*          data,
                                uint16_t          size)
{
    if (handle == NULL || !handle->is_initialized || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    /* Запис адреси регістра, потім читання з repeated start */
    i2c_transfer7(i2c, to_7bit(dev_address), &reg_address, 1, data, size);

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_WriteRegByte(HWD_I2C_Handle_t* handle,
                                     uint16_t          dev_address,
                                     uint8_t           reg_address,
                                     uint8_t           value)
{
    return HWD_I2C_WriteReg(handle, dev_address, reg_address, &value, 1);
}

Servo_Status_t HWD_I2C_ReadRegByte(HWD_I2C_Handle_t* handle,
                                    uint16_t          dev_address,
                                    uint8_t           reg_address,
                                    uint8_t*          value)
{
    if (value == NULL) {
        return SERVO_INVALID;
    }

    return HWD_I2C_ReadReg(handle, dev_address, reg_address, value, 1);
}

Servo_Status_t HWD_I2C_IsDeviceReady(HWD_I2C_Handle_t* handle,
                                      uint16_t          dev_address,
                                      uint8_t           trials)
{
    if (handle == NULL || !handle->is_initialized) {
        return SERVO_INVALID;
    }

    if (trials == 0) {
        trials = 1;
    }

    uint32_t i2c   = (uint32_t)(uintptr_t)handle->config.hw_handle;
    uint8_t  dummy = 0;

    for (uint8_t i = 0; i < trials; i++) {
        /*
         * Спроба читання 1 байта — якщо пристрій відповідає ACK,
         * i2c_transfer7() завершиться без зависання.
         * libopencm3 не повертає статус помилки безпосередньо з transfer7,
         * тому перевіряємо прапор NACK через статус шини.
         */
        i2c_transfer7(i2c, to_7bit(dev_address), NULL, 0, &dummy, 1);

        /* Перевірка прапора помилки підтвердження (AF bit у SR1) */
        if (!(I2C_SR1(i2c) & I2C_SR1_AF)) {
            return SERVO_OK;
        }

        /* Скидання прапора AF перед наступною спробою */
        I2C_SR1(i2c) &= ~I2C_SR1_AF;
    }

    return SERVO_ERROR;
}

#endif /* USE_HWD_I2C */
