/**
 * @file board.c
 * @brief Ініціалізація апаратного забезпечення плати STM32F411
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include "../../Inc/core.h"

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Board_Init(void)
{
    // Ініціалізація HAL виконується в main.c через MX_xxx_Init()
    // Тут можна додати додаткову ініціалізацію якщо потрібно

    return SERVO_OK;
}

Servo_Status_t Board_DeInit(void)
{
    // Деініціалізація периферії якщо потрібно

    return SERVO_OK;
}
