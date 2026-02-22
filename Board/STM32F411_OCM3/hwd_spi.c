/**
 * @file hwd_spi.c
 * @brief Реалізація HWD SPI для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * SPI абстракція через libopencm3.
 *
 * Зберігання в HWD_SPI_Config_t:
 *   spi_handle → (void*)(uintptr_t)SPI1   — базова адреса SPI
 *   cs_port    → (void*)(uintptr_t)GPIOA  — базова адреса GPIO порту CS
 *   cs_pin     → GPIO4                    — бітова маска піна CS
 *
 * Передумова: Board_Init() вже ініціалізував SPI1 та GPIO.
 * CS керується вручну через gpio_set/gpio_clear.
 *
 * Передача: spi_xfer() побайтово (блокуюча, без timeout).
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"

#ifdef USE_HWD_SPI

#include "hwd/hwd_spi.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <string.h>

/* Private functions ---------------------------------------------------------*/

static inline bool spi_is_valid(const HWD_SPI_Handle_t* handle)
{
    return (handle != NULL &&
            handle->config.spi_handle != NULL &&
            handle->config.cs_port    != NULL &&
            handle->is_initialized);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_SPI_Init(HWD_SPI_Handle_t* handle, const HWD_SPI_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->spi_handle == NULL || config->cs_port == NULL) {
        return SERVO_INVALID;
    }

    memcpy(&handle->config, config, sizeof(HWD_SPI_Config_t));

    /* SPI вже налаштований у Board_Init() — встановлюємо CS в неактивний стан */
    HWD_SPI_CS_High(handle);

    handle->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t HWD_SPI_DeInit(HWD_SPI_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    HWD_SPI_CS_High(handle);

    handle->is_initialized = false;

    return SERVO_OK;
}

Servo_Status_t HWD_SPI_TransmitReceive(HWD_SPI_Handle_t* handle,
                                        const uint8_t*    tx_data,
                                        uint8_t*          rx_data,
                                        uint16_t          size)
{
    if (!spi_is_valid(handle)) {
        return SERVO_INVALID;
    }

    if (size == 0 || (tx_data == NULL && rx_data == NULL)) {
        return SERVO_INVALID;
    }

    uint32_t spi = (uint32_t)(uintptr_t)handle->config.spi_handle;

    for (uint16_t i = 0; i < size; i++) {
        uint8_t tx_byte = (tx_data != NULL) ? tx_data[i] : 0xFF;
        uint8_t rx_byte = (uint8_t)spi_xfer(spi, tx_byte);

        if (rx_data != NULL) {
            rx_data[i] = rx_byte;
        }
    }

    return SERVO_OK;
}

Servo_Status_t HWD_SPI_Transmit(HWD_SPI_Handle_t* handle,
                                 const uint8_t*    tx_data,
                                 uint16_t          size)
{
    return HWD_SPI_TransmitReceive(handle, tx_data, NULL, size);
}

Servo_Status_t HWD_SPI_Receive(HWD_SPI_Handle_t* handle,
                                uint8_t*          rx_data,
                                uint16_t          size)
{
    return HWD_SPI_TransmitReceive(handle, NULL, rx_data, size);
}

void HWD_SPI_CS_Low(HWD_SPI_Handle_t* handle)
{
    if (handle != NULL && handle->config.cs_port != NULL) {
        gpio_clear((uint32_t)(uintptr_t)handle->config.cs_port,
                   handle->config.cs_pin);
    }
}

void HWD_SPI_CS_High(HWD_SPI_Handle_t* handle)
{
    if (handle != NULL && handle->config.cs_port != NULL) {
        gpio_set((uint32_t)(uintptr_t)handle->config.cs_port,
                 handle->config.cs_pin);
    }
}

bool HWD_SPI_IsInitialized(const HWD_SPI_Handle_t* handle)
{
    if (handle == NULL) {
        return false;
    }

    return handle->is_initialized;
}

#endif /* USE_HWD_SPI */
