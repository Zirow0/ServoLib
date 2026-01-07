/**
 * @file pid_mgr.h
 * @brief Менеджер PID регуляторів
 * @author ServoCore Team
 * @date 2025
 *
 * Керування кількома PID регуляторами (каскадне керування)
 */

#ifndef SERVOCORE_CTRL_PID_MGR_H
#define SERVOCORE_CTRL_PID_MGR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "pid.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Менеджер каскадних PID регуляторів
 *
 * Реалізує каскадне управління, де вихід одного регулятора
 * є уставкою для наступного:
 *
 * Setpoint → [PID1] → output1 → [PID2] → output2 → [PID3] → final_output
 *             ↑ input1           ↑ input2           ↑ input3
 *
 * Приклади використання:
 * - 2 контролери: Position → Velocity → PWM
 * - 3 контролери: Position → Velocity → Current → PWM
 */
typedef struct {
    PID_Controller_t* pid1;  /**< Зовнішній контур (наприклад, позиція) */
    PID_Controller_t* pid2;  /**< Внутрішній контур (наприклад, швидкість) */
    PID_Controller_t* pid3;  /**< Найглибший контур (опціонально, наприклад струм) */

    uint8_t cascade_depth;   /**< Глибина каскаду: 2 або 3 контролери */
    bool is_initialized;     /**< Прапорець ініціалізації */
} PID_Manager_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація каскаду з 2 PID контролерів
 *
 * @param mgr Вказівник на менеджер
 * @param pid1 Зовнішній контур (наприклад, позиція)
 * @param pid2 Внутрішній контур (наприклад, швидкість)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Manager_Init2(PID_Manager_t* mgr,
                                  PID_Controller_t* pid1,
                                  PID_Controller_t* pid2);

/**
 * @brief Ініціалізація каскаду з 3 PID контролерів
 *
 * @param mgr Вказівник на менеджер
 * @param pid1 Зовнішній контур (наприклад, позиція)
 * @param pid2 Середній контур (наприклад, швидкість)
 * @param pid3 Внутрішній контур (наприклад, струм)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Manager_Init3(PID_Manager_t* mgr,
                                  PID_Controller_t* pid1,
                                  PID_Controller_t* pid2,
                                  PID_Controller_t* pid3);

/**
 * @brief Оновлення каскаду з 2 контролерів
 *
 * Логіка:
 *   output1 = PID1(setpoint, input1)
 *   output2 = PID2(output1, input2)
 *   return output2
 *
 * @param mgr Вказівник на менеджер
 * @param setpoint Бажане значення для PID1 (наприклад, цільова позиція)
 * @param input1 Виміряне значення для PID1 (наприклад, поточна позиція)
 * @param input2 Виміряне значення для PID2 (наприклад, поточна швидкість)
 * @param current_time_us Поточний час в мікросекундах
 * @return float Вихід з останнього контролера (PID2)
 */
float PID_Manager_Update2(PID_Manager_t* mgr,
                          float setpoint,
                          float input1,
                          float input2,
                          uint32_t current_time_us);

/**
 * @brief Оновлення каскаду з 3 контролерів
 *
 * Логіка:
 *   output1 = PID1(setpoint, input1)
 *   output2 = PID2(output1, input2)
 *   output3 = PID3(output2, input3)
 *   return output3
 *
 * @param mgr Вказівник на менеджер
 * @param setpoint Бажане значення для PID1 (наприклад, цільова позиція)
 * @param input1 Виміряне значення для PID1 (наприклад, поточна позиція)
 * @param input2 Виміряне значення для PID2 (наприклад, поточна швидкість)
 * @param input3 Виміряне значення для PID3 (наприклад, поточний струм)
 * @param current_time_us Поточний час в мікросекундах
 * @return float Вихід з останнього контролера (PID3)
 */
float PID_Manager_Update3(PID_Manager_t* mgr,
                          float setpoint,
                          float input1,
                          float input2,
                          float input3,
                          uint32_t current_time_us);

/**
 * @brief Скидання всіх регуляторів у каскаді
 *
 * @param mgr Вказівник на менеджер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Manager_ResetAll(PID_Manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_PID_MGR_H */
