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

#define MAX_PID_CONTROLLERS 3

/**
 * @brief Менеджер PID регуляторів
 */
typedef struct {
    PID_Controller_t* controllers[MAX_PID_CONTROLLERS];  /**< Масив контролерів */
    uint8_t count;                                       /**< Кількість контролерів */
    bool is_initialized;                                 /**< Прапорець ініціалізації */
} PID_Manager_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація менеджера
 */
Servo_Status_t PID_Manager_Init(PID_Manager_t* mgr);

/**
 * @brief Додавання PID регулятора
 */
Servo_Status_t PID_Manager_Add(PID_Manager_t* mgr, PID_Controller_t* pid);

/**
 * @brief Оновлення всіх регуляторів
 */
Servo_Status_t PID_Manager_UpdateAll(PID_Manager_t* mgr);

/**
 * @brief Скидання всіх регуляторів
 */
Servo_Status_t PID_Manager_ResetAll(PID_Manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_PID_MGR_H */
