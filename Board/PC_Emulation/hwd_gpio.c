/**
 * @file hwd_gpio.c
 * @brief Реалізація HWD GPIO для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить емуляцію GPIO для емуляції на ПК через UDP.
 */

/* Includes ------------------------------------------------------------------*/
#include "../hwd/hwd_gpio.h"
#include "board_config.h"
#include "../hwd/hwd_udp.h"
#include "../../Emulator/udp_client.h"
#include <stdio.h>
#include <stdbool.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_GPIO_InitPin(const HWD_GPIO_Pin_t* pin)
{
    if (!pin) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Для емуляції просто реєструємо пін
    printf("GPIO pin initialized (port: %p, pin: %d, mode: %d)\n", 
           pin->port, pin->pin, pin->mode);
    
    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_WritePin(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t state)
{
    if (!pin) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // У емуляції GPIO використовується для керування гальмами через UDP
    // Перевіряємо, чи це пін гальм (спрощена перевірка)
    bool is_brake_pin = (pin->pin == 0); // Умовна перевірка - в реальному коді було б інше
    
    if (is_brake_pin) {
        // Надсилаємо команду гальмам через UDP
        Servo_Status_t status = UDP_Client_SendBrakeCommand(state == HWD_GPIO_PIN_SET);
        if (status != SERVO_OK) {
            printf("ERROR: Failed to send brake command via UDP\n");
            return status;
        }
    }
    
    printf("GPIO pin %d set to %s\n", pin->pin, 
           state == HWD_GPIO_PIN_SET ? "HIGH" : "LOW");
    
    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_ReadPin(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t* state)
{
    if (!pin || !state) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // У емуляції повертаємо фіктивне значення
    *state = HWD_GPIO_PIN_RESET;
    
    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_TogglePin(const HWD_GPIO_Pin_t* pin)
{
    if (!pin) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // У емуляції читаємо поточний стан і інвертуємо його
    HWD_GPIO_PinState_t current_state;
    Servo_Status_t status = HWD_GPIO_ReadPin(pin, &current_state);
    if (status != SERVO_OK) {
        return status;
    }
    
    HWD_GPIO_PinState_t new_state = 
        (current_state == HWD_GPIO_PIN_SET) ? HWD_GPIO_PIN_RESET : HWD_GPIO_PIN_SET;
    
    return HWD_GPIO_WritePin(pin, new_state);
}

Servo_Status_t HWD_GPIO_DeInitPin(const HWD_GPIO_Pin_t* pin)
{
    if (!pin) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    return SERVO_OK;
}