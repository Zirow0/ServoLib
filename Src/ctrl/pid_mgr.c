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

Servo_Status_t PID_Manager_Init2(PID_Manager_t* mgr,
                                  PID_Controller_t* pid1,
                                  PID_Controller_t* pid2)
{
    if (mgr == NULL || pid1 == NULL || pid2 == NULL) {
        return SERVO_INVALID;
    }

    memset(mgr, 0, sizeof(PID_Manager_t));

    mgr->pid1 = pid1;
    mgr->pid2 = pid2;
    mgr->pid3 = NULL;
    mgr->cascade_depth = 2;
    mgr->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t PID_Manager_Init3(PID_Manager_t* mgr,
                                  PID_Controller_t* pid1,
                                  PID_Controller_t* pid2,
                                  PID_Controller_t* pid3)
{
    if (mgr == NULL || pid1 == NULL || pid2 == NULL || pid3 == NULL) {
        return SERVO_INVALID;
    }

    memset(mgr, 0, sizeof(PID_Manager_t));

    mgr->pid1 = pid1;
    mgr->pid2 = pid2;
    mgr->pid3 = pid3;
    mgr->cascade_depth = 3;
    mgr->is_initialized = true;

    return SERVO_OK;
}

float PID_Manager_Update2(PID_Manager_t* mgr,
                          float setpoint,
                          float input1,
                          float input2,
                          uint32_t current_time_us)
{
    if (mgr == NULL || !mgr->is_initialized || mgr->cascade_depth != 2) {
        return 0.0f;
    }

    // Зовнішній контур: setpoint → output1
    PID_Compute(mgr->pid1, setpoint, input1, current_time_us);
    float output1 = PID_GetOutput(mgr->pid1);

    // Внутрішній контур: output1 → output2
    PID_Compute(mgr->pid2, output1, input2, current_time_us);
    float output2 = PID_GetOutput(mgr->pid2);

    return output2;
}

float PID_Manager_Update3(PID_Manager_t* mgr,
                          float setpoint,
                          float input1,
                          float input2,
                          float input3,
                          uint32_t current_time_us)
{
    if (mgr == NULL || !mgr->is_initialized || mgr->cascade_depth != 3) {
        return 0.0f;
    }

    // Зовнішній контур: setpoint → output1
    PID_Compute(mgr->pid1, setpoint, input1, current_time_us);
    float output1 = PID_GetOutput(mgr->pid1);

    // Середній контур: output1 → output2
    PID_Compute(mgr->pid2, output1, input2, current_time_us);
    float output2 = PID_GetOutput(mgr->pid2);

    // Внутрішній контур: output2 → output3
    PID_Compute(mgr->pid3, output2, input3, current_time_us);
    float output3 = PID_GetOutput(mgr->pid3);

    return output3;
}

Servo_Status_t PID_Manager_ResetAll(PID_Manager_t* mgr)
{
    if (mgr == NULL || !mgr->is_initialized) {
        return SERVO_INVALID;
    }

    PID_Reset(mgr->pid1);
    PID_Reset(mgr->pid2);

    if (mgr->cascade_depth == 3 && mgr->pid3 != NULL) {
        PID_Reset(mgr->pid3);
    }

    return SERVO_OK;
}
