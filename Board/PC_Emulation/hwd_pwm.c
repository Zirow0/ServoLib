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
    handle->initialized = true;
    
    printf("HWD PWM initialized with frequency: %.2f Hz\n", config->frequency);
    
    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDuty(HWD_PWM_Handle_t* handle, float duty_cycle)
{
    if (!handle || !handle->initialized) {
        return SERVO_NOT_INIT;
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
    
    // Оновлюємо значення в об'єкту
    handle->duty_cycle = duty_cycle;
    
    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle)
{
    if (!handle || !handle->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Для емуляції просто встановлюємо активний стан
    handle->active = true;
    
    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Stop(HWD_PWM_Handle_t* handle)
{
    if (!handle || !handle->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Зупиняємо PWM, надсилаючи 0% duty cycle
    Servo_Status_t status = HWD_PWM_SetDuty(handle, 0.0f);
    if (status != SERVO_OK) {
        return status;
    }
    
    handle->active = false;
    
    return SERVO_OK;
}

Servo_Status_t HWD_PWM_DeInit(HWD_PWM_Handle_t* handle)
{
    if (!handle) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Зупиняємо PWM перед деініціалізацією
    HWD_PWM_Stop(handle);
    
    handle->initialized = false;
    handle->active = false;
    
    return SERVO_OK;
}