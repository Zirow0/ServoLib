/**
 * @file hwd.c
 * @brief Реалізація єдиного фасаду для всіх HWD модулів
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію функцій для ініціалізації всіх HWD модулів.
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd.h"
#include "hwd_pwm.h"
#include "hwd_i2c.h"
#include "hwd_timer.h"
#include "hwd_gpio.h"
#include "hwd_udp.h"
#include "../Board/PC_Emulation/board_config.h"

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static HWD_Status_t hwd_status = {0};

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_Init(void)
{
    Servo_Status_t status;
    
    // Ініціалізація таймера (має бути першим)
    status = HWD_Timer_Init();
    if (status != SERVO_OK) {
        return status;
    }
    hwd_status.timer_initialized = true;
    
    // Ініціалізація GPIO
    hwd_status.gpio_initialized = true; // В емуляції може бути просто прапорцем
    
    // Ініціалізація PWM
    hwd_status.pwm_initialized = true;  // В емуляції ініціалізується при використанні
    
    // Ініціалізація I2C
    hwd_status.i2c_initialized = true;  // В емуляції може бути вимкненим
    
    // Ініціалізація UDP (для емуляції)
    status = HWD_UDP_Init(UDP_SERVER_IP, UDP_SERVER_PORT, UDP_CLIENT_PORT);
    if (status == SERVO_OK) {
        hwd_status.udp_initialized = true;
    } else {
        hwd_status.udp_initialized = false;
        // Не повертати помилку, якщо UDP не вдається ініціалізувати, бо може бути необхідно пізніше
    }
    
    return SERVO_OK;
}

Servo_Status_t HWD_DeInit(void)
{
    Servo_Status_t status = SERVO_OK;
    
    // Деініціалізація UDP
    if (hwd_status.udp_initialized) {
        Servo_Status_t udp_status = HWD_UDP_DeInit();
        if (udp_status != SERVO_OK) {
            status = udp_status; // Зберігаємо статус, але продовжуємо
        }
        hwd_status.udp_initialized = false;
    }
    
    // Інші деініціалізації
    hwd_status.pwm_initialized = false;
    hwd_status.i2c_initialized = false;
    hwd_status.timer_initialized = false;
    hwd_status.gpio_initialized = false;
    
    return status;
}

Servo_Status_t HWD_GetStatus(HWD_Status_t* status)
{
    if (!status) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    *status = hwd_status;
    return SERVO_OK;
}