/**
 * @file gpio_brake.c
 * @brief Реалізація GPIO драйвера електронних гальм
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Компілювати цей файл тільки якщо використовується brake драйвер */
#ifdef USE_BRAKE

#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_gpio.h"
#include <string.h>

/* Private functions (Hardware Callbacks) ------------------------------------*/

/**
 * @brief Hardware callback: ініціалізація GPIO
 */
static Servo_Status_t GPIO_Brake_HW_Init(void* driver_data)
{
    // GPIO вже ініціалізований в Board layer (CubeMX)
    // Тут можна додати додаткову конфігурацію якщо потрібно

    (void)driver_data;  // Unused в цій реалізації

    return SERVO_OK;
}

/**
 * @brief Hardware callback: фізична активація гальм
 */
static Servo_Status_t GPIO_Brake_HW_Engage(void* driver_data)
{
    GPIO_Brake_Driver_t* driver = (GPIO_Brake_Driver_t*)driver_data;

    if (driver == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Встановлення активного стану GPIO
    HWD_GPIO_PinState_t state = driver->active_high ? HWD_GPIO_PIN_SET : HWD_GPIO_PIN_RESET;

    return HWD_GPIO_WritePin(driver->gpio_port, driver->gpio_pin, state);
}

/**
 * @brief Hardware callback: фізичне відпускання гальм
 */
static Servo_Status_t GPIO_Brake_HW_Release(void* driver_data)
{
    GPIO_Brake_Driver_t* driver = (GPIO_Brake_Driver_t*)driver_data;

    if (driver == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Встановлення неактивного стану GPIO
    HWD_GPIO_PinState_t state = driver->active_high ? HWD_GPIO_PIN_RESET : HWD_GPIO_PIN_SET;

    return HWD_GPIO_WritePin(driver->gpio_port, driver->gpio_pin, state);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t GPIO_Brake_Create(GPIO_Brake_Driver_t* driver, const GPIO_Brake_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (config->gpio_port == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Очистка структури
    memset(driver, 0, sizeof(GPIO_Brake_Driver_t));

    // Збереження GPIO конфігурації
    driver->gpio_port = config->gpio_port;
    driver->gpio_pin = config->gpio_pin;
    driver->active_high = config->active_high;

    // Налаштування hardware callbacks
    driver->interface.hw.init    = GPIO_Brake_HW_Init;
    driver->interface.hw.engage  = GPIO_Brake_HW_Engage;
    driver->interface.hw.release = GPIO_Brake_HW_Release;

    // Вказівник на driver_data (для callbacks)
    driver->interface.driver_data = driver;

    // Ініціалізація базового інтерфейсу з таймінгами
    Brake_Params_t params = {
        .engage_time_ms = config->engage_time_ms,
        .release_time_ms = config->release_time_ms
    };

    return Brake_Init(&driver->interface, &params);
}

#endif /* USE_BRAKE */
