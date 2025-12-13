/**
 * @file hwd_pwm.c
 * @brief Реалізація HWD PWM для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить емуляцію PWM через UDP для емуляції на ПК.
 */

/* Includes ------------------------------------------------------------------*/
#include "../hwd/hwd_pwm.h"
#include "board_config.h"
#include "../hwd/hwd_udp.h"
#include "../../Emulator/udp_client.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_PWM_Init(HWD_PWM_Handle_t* handle, const HWD_PWM_Config_t* config)
{
    if (!handle || !config) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Для емуляції на ПК просто зберігаємо конфігурацію
    handle->config = *config;
    handle->current_duty = 0;
    handle->mode = HWD_PWM_MODE_FORWARD;
    handle->is_running = false;

    printf("HWD PWM initialized with frequency: %u Hz\n", config->frequency);

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float duty_cycle)
{
    if (!handle) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Обмеження duty cycle в межах [0, 100]
    if (duty_cycle < 0.0f) {
        duty_cycle = 0.0f;
    } else if (duty_cycle > 100.0f) {
        duty_cycle = 100.0f;
    }
    
    // В емуляції PWM значення передається як потужність двигуна
    // Значення від -100 до +100 через UDP
    float power = duty_cycle - 50.0f; // Перетворюємо 0-100 в -50 до +50
    if (duty_cycle > 50.0f) {
        power = (duty_cycle - 50.0f) * 2.0f; // 50-100 => 0-100
    } else if (duty_cycle < 50.0f) {
        power = (duty_cycle - 50.0f) * 2.0f; // 0-50 => -100-0
    } else {
        power = 0.0f; // 50 = нейтральна точка
    }
    
    // Надсилаємо команду двигуну через UDP
    Servo_Status_t status = UDP_Client_SendMotorCommand(power);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to send PWM duty cycle via UDP: %.2f (power: %.2f)\n", duty_cycle, power);
        return status;
    }
    
    // Оновлюємо значення в об'єкту (конвертуємо відсотки в абсолютні значення)
    handle->current_duty = (uint32_t)((duty_cycle / 100.0f) * handle->config.resolution);

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle)
{
    if (!handle) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Для емуляції просто встановлюємо активний стан
    handle->is_running = true;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Stop(HWD_PWM_Handle_t* handle)
{
    if (!handle) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Зупиняємо PWM, надсилаючи 0% duty cycle
    Servo_Status_t status = HWD_PWM_SetDutyPercent(handle, 0.0f);
    if (status != SERVO_OK) {
        return status;
    }

    handle->is_running = false;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_DeInit(HWD_PWM_Handle_t* handle)
{
    if (!handle) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Зупиняємо PWM перед деініціалізацією
    HWD_PWM_Stop(handle);

    handle->is_running = false;
    handle->current_duty = 0;

    return SERVO_OK;
}