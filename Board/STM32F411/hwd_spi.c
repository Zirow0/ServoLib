/**
 * @file hwd_spi.c
 * @brief Реалізація HWD SPI для STM32F411
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізація SPI абстракції через STM32 HAL для STM32F411CEU6
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"

#ifdef USE_HWD_SPI

#include "hwd/hwd_spi.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Перевірка валідності конфігурації
 */
static inline bool hwd_spi_is_valid(const HWD_SPI_Handle_t* handle)
{
    return (handle != NULL &&
            handle->config.spi_handle != NULL &&
            handle->config.cs_port != NULL);
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

    // Копіювати конфігурацію
    memcpy(&handle->config, config, sizeof(HWD_SPI_Config_t));

    // Встановити CS в HIGH (неактивний стан)
    HWD_SPI_CS_High(handle);

    handle->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t HWD_SPI_DeInit(HWD_SPI_Handle_t* handle)
{
    if (!hwd_spi_is_valid(handle)) {
        return SERVO_INVALID;
    }

    // Встановити CS в HIGH
    HWD_SPI_CS_High(handle);

    handle->is_initialized = false;

    return SERVO_OK;
}

Servo_Status_t HWD_SPI_TransmitReceive(HWD_SPI_Handle_t* handle,
                                        const uint8_t* tx_data,
                                        uint8_t* rx_data,
                                        uint16_t size)
{
    if (!hwd_spi_is_valid(handle)) {
        return SERVO_INVALID;
    }

    if (size == 0) {
        return SERVO_INVALID;
    }

    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)handle->config.spi_handle;
    HAL_StatusTypeDef hal_status;

    if (tx_data != NULL && rx_data != NULL) {
        // Full duplex - передача та прийом одночасно
        hal_status = HAL_SPI_TransmitReceive(hspi,
                                              (uint8_t*)tx_data,
                                              rx_data,
                                              size,
                                              handle->config.timeout_ms);
    } else if (tx_data != NULL) {
        // Тільки передача
        hal_status = HAL_SPI_Transmit(hspi,
                                       (uint8_t*)tx_data,
                                       size,
                                       handle->config.timeout_ms);
    } else if (rx_data != NULL) {
        // Тільки прийом
        hal_status = HAL_SPI_Receive(hspi,
                                      rx_data,
                                      size,
                                      handle->config.timeout_ms);
    } else {
        return SERVO_INVALID;
    }

    // Конвертувати HAL статус в Servo статус
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

Servo_Status_t HWD_SPI_Transmit(HWD_SPI_Handle_t* handle,
                                 const uint8_t* tx_data,
                                 uint16_t size)
{
    return HWD_SPI_TransmitReceive(handle, tx_data, NULL, size);
}

Servo_Status_t HWD_SPI_Receive(HWD_SPI_Handle_t* handle,
                                uint8_t* rx_data,
                                uint16_t size)
{
    return HWD_SPI_TransmitReceive(handle, NULL, rx_data, size);
}

void HWD_SPI_CS_Low(HWD_SPI_Handle_t* handle)
{
    if (hwd_spi_is_valid(handle)) {
        HAL_GPIO_WritePin((GPIO_TypeDef*)handle->config.cs_port,
                          handle->config.cs_pin,
                          GPIO_PIN_RESET);
    }
}

void HWD_SPI_CS_High(HWD_SPI_Handle_t* handle)
{
    if (hwd_spi_is_valid(handle)) {
        HAL_GPIO_WritePin((GPIO_TypeDef*)handle->config.cs_port,
                          handle->config.cs_pin,
                          GPIO_PIN_SET);
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
