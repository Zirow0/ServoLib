/**
 * @file hwd.h
 * @brief Єдиний фасад для всіх HWD модулів
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл об'єднує всі HWD-інтерфейси в один entry point.
 * Використовуйте цей файл для включення всієї HWD-абстракції.
 */

#ifndef SERVOCORE_HWD_H
#define SERVOCORE_HWD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "./hwd_pwm.h"
#include "./hwd_i2c.h"
#include "./hwd_timer.h"
#include "./hwd_gpio.h"
#include "./hwd_udp.h"  /**< Додано для емуляції */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура глобальної HWD конфігурації
 */
typedef struct {
    bool pwm_initialized;    /**< PWM ініціалізовано */
    bool i2c_initialized;    /**< I2C ініціалізовано */
    bool timer_initialized;  /**< Timer ініціалізовано */
    bool gpio_initialized;   /**< GPIO ініціалізовано */
    bool udp_initialized;    /**< UDP ініціалізовано (для емуляції) */
} HWD_Status_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація всього HWD
 *
 * Викликає ініціалізацію всіх HWD модулів.
 * Має викликатися один раз при старті системи.
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_Init(void);

/**
 * @brief Деініціалізація всього HWD
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_DeInit(void);

/**
 * @brief Отримання статусу HWD
 *
 * @param status Вказівник для збереження статусу
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GetStatus(HWD_Status_t* status);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_H */
