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
} HWD_I2C_Speed_t;

/**
 * @brief Конфігурація I2C
 */
typedef struct {
    HWD_I2C_Speed_t speed;  /**< Швидкість */
    void*           hw_handle; /**< Базова адреса I2C периферії (платформо-специфічна) */
} HWD_I2C_Config_t;

/**
 * @brief Дескриптор I2C
 */
typedef struct {
    HWD_I2C_Config_t config; /**< Конфігурація */
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

Servo_Status_t HWD_I2C_ReadReg(HWD_I2C_Handle_t* handle,
                                uint16_t dev_address,
                                uint8_t  reg_address,
                                uint8_t* data,
                                uint16_t size);

Servo_Status_t HWD_I2C_ReadRegByte(HWD_I2C_Handle_t* handle,
                                    uint16_t dev_address,
                                    uint8_t  reg_address,
                                    uint8_t* value);

Servo_Status_t HWD_I2C_IsDeviceReady(HWD_I2C_Handle_t* handle,
                                      uint16_t dev_address,
                                      uint8_t  trials);

/**
 * @brief Запуск фонового циклічного читання через I2C IT
 *
 * Підтримує size = 1 або 2 байти. DMA буфер оновлюється автоматично в ISR.
 * Викликати HWD_I2C_EV_Handler() з i2c1_ev_isr у board коді.
 *
 * @param handle      Вказівник на дескриптор I2C
 * @param dev_address Адреса пристрою (8-bit: 7-bit addr << 1)
 * @param reg_address Адреса регістру для читання
 * @param buf         volatile буфер для результату
 * @param size        Розмір даних (1 або 2 байти)
 * @return Servo_Status_t Статус ініціалізації
 */
Servo_Status_t HWD_I2C_StartContinuousRead(HWD_I2C_Handle_t* handle,
                                            uint16_t          dev_address,
                                            uint8_t           reg_address,
                                            volatile uint8_t* buf,
                                            uint16_t          size);

/**
 * @brief Обробник I2C EV переривання для фонового читання
 *
 * i2c1_ev_isr визначений у hwd_i2c.c і викликає цю функцію.
 * Якщо потрібен інший I2C — викликати вручну з відповідного ISR.
 */
void HWD_I2C_EV_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_I2C_H */
