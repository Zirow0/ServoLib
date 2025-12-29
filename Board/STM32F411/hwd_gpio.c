/**
 * @file hwd_gpio.c
 * @brief Реалізація HWD GPIO для STM32F411
 * @author ServoCore Team
 * @date 2025
 *
 * Платформо-специфічна реалізація GPIO для STM32F4xx.
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_gpio.h"
#include "./board_config.h"

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_GPIO_WritePin(void* port, uint16_t pin, HWD_GPIO_PinState_t state)
{
    if (port == NULL) {
        return SERVO_INVALID;
    }

    GPIO_TypeDef* gpio_port = (GPIO_TypeDef*)port;
    GPIO_PinState hal_state = (state == HWD_GPIO_PIN_SET) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(gpio_port, pin, hal_state);

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_ReadPin(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t* state)
{
    if (pin == NULL || pin->port == NULL || state == NULL) {
        return SERVO_INVALID;
    }

    GPIO_TypeDef* gpio_port = (GPIO_TypeDef*)pin->port;

    GPIO_PinState hal_state = HAL_GPIO_ReadPin(gpio_port, pin->pin);

    *state = (hal_state == GPIO_PIN_SET) ? HWD_GPIO_PIN_SET : HWD_GPIO_PIN_RESET;

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_TogglePin(const HWD_GPIO_Pin_t* pin)
{
    if (pin == NULL || pin->port == NULL) {
        return SERVO_INVALID;
    }

    GPIO_TypeDef* gpio_port = (GPIO_TypeDef*)pin->port;

    HAL_GPIO_TogglePin(gpio_port, pin->pin);

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_InitPin(const HWD_GPIO_Pin_t* pin)
{
    if (pin == NULL || pin->port == NULL) {
        return SERVO_INVALID;
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin->pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    // Встановлення режиму
    switch (pin->mode) {
        case HWD_GPIO_MODE_INPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            break;

        case HWD_GPIO_MODE_OUTPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            break;

        case HWD_GPIO_MODE_AF:
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            break;

        case HWD_GPIO_MODE_ANALOG:
            GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
            break;

        default:
            return SERVO_INVALID;
    }

    // Встановлення підтяжки
    switch (pin->pull) {
        case HWD_GPIO_NOPULL:
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            break;

        case HWD_GPIO_PULLUP:
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;

        case HWD_GPIO_PULLDOWN:
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            break;

        default:
            return SERVO_INVALID;
    }

    // Ініціалізація GPIO
    HAL_GPIO_Init((GPIO_TypeDef*)pin->port, &GPIO_InitStruct);

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_DeInitPin(const HWD_GPIO_Pin_t* pin)
{
    if (pin == NULL || pin->port == NULL) {
        return SERVO_INVALID;
    }

    GPIO_TypeDef* gpio_port = (GPIO_TypeDef*)pin->port;

    HAL_GPIO_DeInit(gpio_port, pin->pin);

    return SERVO_OK;
}
