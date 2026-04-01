/**
 * @file hwd_uart.c
 * @brief Реалізація HWD UART для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * USART1: PA9 (TX) / PA10 (RX), AF7, 115200 8N1.
 * APB2 = 100 MHz.
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include "../../Inc/hwd/hwd_uart.h"

#ifdef USE_HWD_UART

#include <libopencm3/stm32/usart.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_UART_WriteByte(uint8_t byte)
{
    usart_send_blocking(UART_DEBUG, byte);
    return SERVO_OK;
}

Servo_Status_t HWD_UART_WriteString(const char* str)
{
    if (str == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    while (*str != '\0') {
        usart_send_blocking(UART_DEBUG, (uint8_t)(*str));
        str++;
    }

    return SERVO_OK;
}

#endif /* USE_HWD_UART */
