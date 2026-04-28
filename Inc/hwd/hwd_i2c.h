/**
 * @file hwd_i2c.h
 * @brief Апаратна абстракція I2C
 * @author ServoCore Team
 * @date 2025
 *
 * Незалежний від платформи інтерфейс для роботи з I2C
 */

#ifndef SERVOCORE_HWD_I2C_H
#define SERVOCORE_HWD_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Швидкість I2C
 */
typedef enum {
    HWD_I2C_SPEED_STANDARD = 0,  /**< 100 kHz */
    HWD_I2C_SPEED_FAST     = 1,  /**< 400 kHz */
    HWD_I2C_SPEED_400K     = 1,  /**< 400 kHz (alias) */
    HWD_I2C_SPEED_FAST_PLUS = 2  /**< 1 MHz */
} HWD_I2C_Speed_t;

/**
 * @brief Конфігурація I2C
 */
typedef struct {
    HWD_I2C_Speed_t speed;   /**< Швидкість */
    void* hw_handle;         /**< Базова адреса I2C периферії (платформо-специфічна) */
    uint32_t timeout_ms;     /**< Таймаут операцій (мс) */
} HWD_I2C_Config_t;

/**
 * @brief Дескриптор I2C
 */
typedef struct {
    HWD_I2C_Config_t config; /**< Конфігурація */
    bool is_initialized;     /**< Прапорець ініціалізації */
} HWD_I2C_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація I2C
 *
 * @param handle Вказівник на дескриптор I2C
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_Init(HWD_I2C_Handle_t* handle, const HWD_I2C_Config_t* config);

/**
 * @brief Деініціалізація I2C
 *
 * @param handle Вказівник на дескриптор I2C
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_DeInit(HWD_I2C_Handle_t* handle);

/**
 * @brief Запис даних в пристрій
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою (7-bit або 8-bit)
 * @param data Вказівник на дані для запису
 * @param size Розмір даних (байт)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_Write(HWD_I2C_Handle_t* handle,
                             uint16_t dev_address,
                             const uint8_t* data,
                             uint16_t size);

/**
 * @brief Читання даних з пристрою
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою (7-bit або 8-bit)
 * @param data Вказівник для збереження даних
 * @param size Розмір даних (байт)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_Read(HWD_I2C_Handle_t* handle,
                            uint16_t dev_address,
                            uint8_t* data,
                            uint16_t size);

/**
 * @brief Запис в регістр пристрою
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою
 * @param reg_address Адреса регістру
 * @param data Вказівник на дані для запису
 * @param size Розмір даних (байт)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_WriteReg(HWD_I2C_Handle_t* handle,
                                uint16_t dev_address,
                                uint8_t reg_address,
                                const uint8_t* data,
                                uint16_t size);

/**
 * @brief Читання з регістру пристрою
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою
 * @param reg_address Адреса регістру
 * @param data Вказівник для збереження даних
 * @param size Розмір даних (байт)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_ReadReg(HWD_I2C_Handle_t* handle,
                               uint16_t dev_address,
                               uint8_t reg_address,
                               uint8_t* data,
                               uint16_t size);

/**
 * @brief Запис одного байту в регістр
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою
 * @param reg_address Адреса регістру
 * @param value Значення для запису
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_WriteRegByte(HWD_I2C_Handle_t* handle,
                                    uint16_t dev_address,
                                    uint8_t reg_address,
                                    uint8_t value);

/**
 * @brief Читання одного байту з регістру
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою
 * @param reg_address Адреса регістру
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_I2C_ReadRegByte(HWD_I2C_Handle_t* handle,
                                   uint16_t dev_address,
                                   uint8_t reg_address,
                                   uint8_t* value);

/**
 * @brief Перевірка доступності пристрою на шині
 *
 * @param handle Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою
 * @param trials Кількість спроб
 * @return Servo_Status_t SERVO_OK якщо пристрій доступний
 */
Servo_Status_t HWD_I2C_IsDeviceReady(HWD_I2C_Handle_t* handle,
                                     uint16_t dev_address,
                                     uint8_t trials);

/**
 * @brief Запуск фонового циклічного читання через I2C IT
 *
 * Ініціює першу транзакцію. Після завершення кожного читання I2C TC IRQ
 * автоматично копіює дані в buf та запускає наступне читання.
 * buf оновлюється безперервно без участі CPU.
 *
 * @param handle      Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою (7-bit << 1)
 * @param reg_address Адреса регістру для читання
 * @param buf         volatile буфер для результату
 * @param size        Розмір даних (байт)
 * @return Servo_Status_t Статус ініціалізації
 */
Servo_Status_t HWD_I2C_StartContinuousRead(HWD_I2C_Handle_t* handle,
                                            uint16_t dev_address,
                                            uint8_t  reg_address,
                                            volatile uint8_t* buf,
                                            uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_I2C_H */
