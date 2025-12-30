/**
 * @file hwd.c
 * @brief Реалізація єдиного фасаду для всіх HWD модулів
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію функцій для ініціалізації всіх HWD модулів.
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/hwd/hwd.h"
#include "../../Inc/hwd/hwd_pwm.h"
#include "../../Inc/hwd/hwd_i2c.h"
#include "../../Inc/hwd/hwd_timer.h"
#include "../../Inc/hwd/hwd_gpio.h"

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static HWD_Status_t hwd_status = {0};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_Init(void)
{
    // Таймер ініціалізується HAL, не потребує окремої ініціалізації
    hwd_status.timer_initialized = true;

    // GPIO ініціалізується HAL
    hwd_status.gpio_initialized = true;

    // PWM ініціалізується HAL
    hwd_status.pwm_initialized = true;

    // I2C ініціалізується HAL (опціонально)
    hwd_status.i2c_initialized = true;

    return SERVO_OK;
}

Servo_Status_t HWD_DeInit(void)
{
    // Деініціалізація всіх модулів
    hwd_status.pwm_initialized = false;
    hwd_status.i2c_initialized = false;
    hwd_status.timer_initialized = false;
    hwd_status.gpio_initialized = false;

    return SERVO_OK;
}

Servo_Status_t HWD_GetStatus(HWD_Status_t* status)
{
    if (!status) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    *status = hwd_status;
    return SERVO_OK;
}