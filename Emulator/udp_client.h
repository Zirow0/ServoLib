/**
 * @file udp_client.h
 * @brief UDP клієнт для емуляції сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить функції для UDP комунікації з математичною моделлю.
 */

#ifndef SERVOCORE_EMULATOR_UDP_CLIENT_H
#define SERVOCORE_EMULATOR_UDP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../Inc/core.h"
#include "../Inc/hwd/hwd_udp.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація UDP клієнта
 * 
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_Init(void);

/**
 * @brief Відправлення команди двигуну
 * 
 * @param power Потужність (-100.0 до +100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_SendMotorCommand(float power);

/**
 * @brief Отримання статусу двигуна
 * 
 * @param[out] position Поточна позиція (градуси)
 * @param[out] velocity Поточна швидкість (град/с)
 * @param[out] current Струм (mA)
 * @param[out] stalled Чи заклинений двигун
 * @param[out] overcurrent Чи перевищено струм
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_GetMotorStatus(float* position, float* velocity, 
                                         float* current, bool* stalled, bool* overcurrent);

/**
 * @brief Відправлення команди гальмам
 * 
 * @param engaged Активувати/деактивувати гальма
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_SendBrakeCommand(bool engaged);

/**
 * @brief Отримання статусу гальм
 * 
 * @param[out] engaged Чи гальма активні
 * @param[out] ready Чи гальма готові
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_GetBrakeStatus(bool* engaged, bool* ready);

/**
 * @brief Відправлення запиту на отримання даних сенсора
 * 
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_RequestSensorData(void);

/**
 * @brief Отримання даних сенсора
 * 
 * @param[out] angle Кут у градусах
 * @param[out] velocity Швидкість (град/с)
 * @param[out] connected Чи сенсор підключено
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_GetSensorData(float* angle, float* velocity, bool* connected);

/**
 * @brief Деініціалізація UDP клієнта
 * 
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_DeInit(void);

/**
 * @brief Перевірка зв'язку з сервером
 * 
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t UDP_Client_Ping(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_EMULATOR_UDP_CLIENT_H */