/**
 * @file hwd_gpio.c
 * @brief Реалізація HWD GPIO для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * GPIO абстракція через libopencm3.
 *
 * Зберігання адреси порту:
 *   void* port містить базову адресу GPIO порту (наприклад, GPIOA = 0x40020000).
 *   При запису: (void*)(uintptr_t)GPIOA
 *   При читанні: (uint32_t)(uintptr_t)pin->port
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_gpio.h"
#include "./board_config.h"
#include <libopencm3/stm32/gpio.h>
#include <stdint.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_GPIO_WritePin(void* port, uint16_t pin, HWD_GPIO_PinState_t state)
{
    if (port == NULL) {
        return SERVO_INVALID;
    }

    uint32_t gpio_port = (uint32_t)(uintptr_t)port;

    if (state == HWD_GPIO_PIN_SET) {
        gpio_set(gpio_port, pin);
    } else {
        gpio_clear(gpio_port, pin);
    }

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_ReadPin(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t* state)
{
    if (pin == NULL || pin->port == NULL || state == NULL) {
        return SERVO_INVALID;
    }

    uint32_t gpio_port = (uint32_t)(uintptr_t)pin->port;

    *state = (gpio_get(gpio_port, pin->pin) != 0)
             ? HWD_GPIO_PIN_SET
             : HWD_GPIO_PIN_RESET;

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_TogglePin(const HWD_GPIO_Pin_t* pin)
{
    if (pin == NULL || pin->port == NULL) {
        return SERVO_INVALID;
    }

    gpio_toggle((uint32_t)(uintptr_t)pin->port, pin->pin);

    return SERVO_OK;
}

Servo_Status_t HWD_GPIO_InitPin(const HWD_GPIO_Pin_t* pin)
{
    if (pin == NULL || pin->port == NULL) {
        return SERVO_INVALID;
    }

    uint32_t gpio_port = (uint32_t)(uintptr_t)pin->port;
    uint8_t  ocm3_mode;
    uint8_t  ocm3_pull;

    /* Конвертація режиму */
    switch (pin->mode) {
        case HWD_GPIO_MODE_INPUT:
            ocm3_mode = GPIO_MODE_INPUT;
            break;
        case HWD_GPIO_MODE_OUTPUT:
            ocm3_mode = GPIO_MODE_OUTPUT;
            break;
        case HWD_GPIO_MODE_AF:
            ocm3_mode = GPIO_MODE_AF;
            break;
        case HWD_GPIO_MODE_ANALOG:
            ocm3_mode = GPIO_MODE_ANALOG;
            break;
        default:
            return SERVO_INVALID;
    }

    /* Конвертація підтяжки */
    switch (pin->pull) {
        case HWD_GPIO_NOPULL:
            ocm3_pull = GPIO_PUPD_NONE;
            break;
        case HWD_GPIO_PULLUP:
            ocm3_pull = GPIO_PUPD_PULLUP;
            break;
        case HWD_GPIO_PULLDOWN:
            ocm3_pull = GPIO_PUPD_PULLDOWN;
            break;
        default:
            return SERVO_INVALID;
    }

    gpio_mode_setup(gpio_port, ocm3_mode, ocm3_pull, pin->pin);

    /* Для виходів і AF: push-pull, low speed */
    if (pin->mode == HWD_GPIO_MODE_OUTPUT || pin->mode == HWD_GPIO_MODE_AF) {
        gpio_set_output_options(gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, pin->pin);
    }

    return SERVO_OK;
}

