/**
 * @file hwd_uart.h
 * @brief Апаратна абстракція UART
 * @author ServoCore Team
 * @date 2025
 *
 * Незалежний від платформи інтерфейс для виведення даних через UART.
 * Реалізація — у Board/YOUR_PLATFORM/hwd_uart.c.
 */

#ifndef SERVOCORE_HWD_UART_H
#define SERVOCORE_HWD_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Передача одного байту через UART (блокуюча)
 *
 * @param byte Байт для передачі
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UART_WriteByte(uint8_t byte);

/**
 * @brief Передача рядка через UART (блокуюча)
 *
 * Передає символи до термінального нуля '\0'.
 *
 * @param str Вказівник на null-terminated рядок
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UART_WriteString(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_UART_H */
