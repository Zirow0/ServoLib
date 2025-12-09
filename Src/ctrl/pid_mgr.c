/**
 * @file pid_mgr.c
 * @brief Реалізація менеджера PID
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/pid_mgr.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PID_Manager_Init(PID_Manager_t* mgr)
{
    if (mgr == NULL) {
        return SERVO_INVALID;
    }

    memset(mgr, 0, sizeof(PID_Manager_t));
    mgr->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t PID_Manager_Add(PID_Manager_t* mgr, PID_Controller_t* pid)
{
    if (mgr == NULL || !mgr->is_initialized || pid == NULL) {
        return SERVO_INVALID;
    }

    if (mgr->count >= MAX_PID_CONTROLLERS) {
        return SERVO_ERROR;
    }

    mgr->controllers[mgr->count++] = pid;

    return SERVO_OK;
}

Servo_Status_t PID_Manager_UpdateAll(PID_Manager_t* mgr)
{
    if (mgr == NULL || !mgr->is_initialized) {
        return SERVO_INVALID;
    }

    // Кожен регулятор оновлюється окремо через Servo_Update

    return SERVO_OK;
}

Servo_Status_t PID_Manager_ResetAll(PID_Manager_t* mgr)
{
    if (mgr == NULL || !mgr->is_initialized) {
        return SERVO_INVALID;
    }

    for (uint8_t i = 0; i < mgr->count; i++) {
        PID_Reset(mgr->controllers[i]);
    }

    return SERVO_OK;
}
