/**
 * @file gpio_brake.h
 * @brief GPIO драйвер електронних гальм
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізація драйвера гальм з керуванням через GPIO для електромагнітних гальм
 */

#ifndef SERVOCORE_DRV_GPIO_BRAKE_H
#define SERVOCORE_DRV_GPIO_BRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "brake.h"
#include "../../hwd/hwd_gpio.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація GPIO драйвера гальм
 */
typedef struct {
    /* GPIO параметри */
    void* gpio_port;            /**< Порт GPIO (GPIO_TypeDef*) */
    uint16_t gpio_pin;          /**< Номер піна */
    bool active_high;           /**< true = активний HIGH, false = активний LOW */

    /* Таймінги переходів (мс) */
    uint32_t engage_time_ms;    /**< Час спрацювання гальм (RELEASING → ENGAGED) */
    uint32_t release_time_ms;   /**< Час відпускання гальм (ENGAGING → RELEASED) */
} GPIO_Brake_Config_t;

/**
 * @brief GPIO драйвер гальм
 *
 * ВАЖЛИВО: Brake_Interface_t повинен бути ПЕРШИМ полем для сумісності
 */
typedef struct {
    Brake_Interface_t interface;    /**< Універсальний інтерфейс (ПЕРШЕ поле!) */

    /* GPIO конфігурація */
    void* gpio_port;                /**< Порт GPIO */
    uint16_t gpio_pin;              /**< Номер піна */
    bool active_high;               /**< Логіка активації */
} GPIO_Brake_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення GPIO драйвера гальм
 *
 * Налаштовує hardware callbacks та зберігає конфігурацію.
 * Після виклику використовуйте &driver->interface для роботи з гальмами.
 *
 * @param driver Вказівник на драйвер
 * @param config Конфігурація GPIO та таймінгів
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t GPIO_Brake_Create(GPIO_Brake_Driver_t* driver, const GPIO_Brake_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_GPIO_BRAKE_H */
