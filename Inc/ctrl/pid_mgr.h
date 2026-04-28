/**
 * @file pid_mgr.h
 * @brief Менеджер каскадних PID регуляторів
 * @author ServoCore Team
 * @date 2025
 *
 * Каскадне керування: вихід зовнішнього контуру є уставкою для внутрішнього.
 *
 *   Setpoint → [PID1] → [PID2] → output          (2 контури, pid3 = NULL)
 *   Setpoint → [PID1] → [PID2] → [PID3] → output  (3 контури)
 *
 * Приклади:
 *   2 контури: Position → Velocity → PWM
 *   3 контури: Position → Velocity → Current → PWM
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
 */
typedef struct {
    PID_Controller_t* pid1;  /**< Зовнішній контур (напр. позиція) */
    PID_Controller_t* pid2;  /**< Середній контур (напр. швидкість) */
    PID_Controller_t* pid3;  /**< Внутрішній контур (напр. струм), NULL = 2 контури */
} PID_Manager_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація каскаду PID регуляторів
 *
 * @param mgr  Вказівник на менеджер
 * @param pid1 Зовнішній контур
 * @param pid2 Середній/внутрішній контур
 * @param pid3 Найглибший контур (NULL для 2-контурного каскаду)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PID_Manager_Init(PID_Manager_t* mgr,
                                 PID_Controller_t* pid1,
                                 PID_Controller_t* pid2,
                                 PID_Controller_t* pid3);

/**
 * @brief Оновлення каскаду
 *
 * Якщо pid3 != NULL — виконує 3 контури, інакше 2.
 * input3 ігнорується якщо pid3 == NULL.
 *
 * @param mgr            Вказівник на менеджер
 * @param setpoint       Уставка для pid1
 * @param input1         Зворотній зв'язок для pid1
 * @param input2         Зворотній зв'язок для pid2
 * @param input3         Зворотній зв'язок для pid3 (ігнорується якщо pid3=NULL)
 * @param current_time_us Поточний час (мкс)
 * @return float         Вихід останнього контуру
 */
float PID_Manager_Update(PID_Manager_t* mgr,
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
