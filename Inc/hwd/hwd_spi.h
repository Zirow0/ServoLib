/**
 * @file hwd_spi.h
 * @brief Апаратна абстракція SPI інтерфейсу
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл визначає незалежний від платформи інтерфейс для роботи з SPI.
 * Конкретна реалізація надається в Board/STM32F411/hwd_spi.c
 */

#ifndef SERVOCORE_HWD_SPI_H
#define SERVOCORE_HWD_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура конфігурації SPI
 */
typedef struct {
    void* spi_handle;       /**< Вказівник на HAL SPI handle (наприклад, &hspi1) */
    void* cs_port;          /**< GPIO порт для CS (Chip Select) */
    uint16_t cs_pin;        /**< GPIO пін для CS */
    uint32_t timeout_ms;    /**< Таймаут операцій (мс) */
} HWD_SPI_Config_t;

/**
 * @brief Структура дескриптора SPI
 */
typedef struct {
    HWD_SPI_Config_t config;  /**< Конфігурація SPI */
    bool is_initialized;      /**< Прапорець ініціалізації */
} HWD_SPI_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація SPI інтерфейсу
 *
 * @param handle Вказівник на дескриптор SPI
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_SPI_Init(HWD_SPI_Handle_t* handle, const HWD_SPI_Config_t* config);

/**
 * @brief Деініціалізація SPI інтерфейсу
 *
 * @param handle Вказівник на дескриптор SPI
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_SPI_DeInit(HWD_SPI_Handle_t* handle);

/**
 * @brief Передача та прийом даних через SPI (Full Duplex)
 *
 * @param handle Вказівник на дескриптор SPI
 * @param tx_data Дані для передачі (NULL якщо тільки прийом)
 * @param rx_data Буфер для прийому (NULL якщо тільки передача)
 * @param size Розмір даних в байтах
 * @return Servo_Status_t Статус операції
 */
Servo_Status_t HWD_SPI_TransmitReceive(HWD_SPI_Handle_t* handle,
                                        const uint8_t* tx_data,
                                        uint8_t* rx_data,
                                        uint16_t size);

/**
 * @brief Тільки передача даних через SPI
 *
 * @param handle Вказівник на дескриптор SPI
 * @param tx_data Дані для передачі
 * @param size Розмір даних в байтах
 * @return Servo_Status_t Статус операції
 */
Servo_Status_t HWD_SPI_Transmit(HWD_SPI_Handle_t* handle,
                                 const uint8_t* tx_data,
                                 uint16_t size);

/**
 * @brief Тільки прийом даних через SPI
 *
 * @param handle Вказівник на дескриптор SPI
 * @param rx_data Буфер для прийому
 * @param size Розмір даних в байтах
 * @return Servo_Status_t Статус операції
 */
Servo_Status_t HWD_SPI_Receive(HWD_SPI_Handle_t* handle,
                                uint8_t* rx_data,
                                uint16_t size);

/**
 * @brief Встановлення CS (Chip Select) в LOW (активний)
 *
 * @param handle Вказівник на дескриптор SPI
 */
void HWD_SPI_CS_Low(HWD_SPI_Handle_t* handle);

/**
 * @brief Встановлення CS (Chip Select) в HIGH (неактивний)
 *
 * @param handle Вказівник на дескриптор SPI
 */
void HWD_SPI_CS_High(HWD_SPI_Handle_t* handle);

/**
 * @brief Перевірка стану SPI інтерфейсу
 *
 * @param handle Вказівник на дескриптор SPI
 * @return bool true якщо ініціалізований, false інакше
 */
bool HWD_SPI_IsInitialized(const HWD_SPI_Handle_t* handle);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_SPI_H */
