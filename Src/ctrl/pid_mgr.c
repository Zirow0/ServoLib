/**
 * @file pid_mgr.c
 * @brief Реалізація менеджера каскадних PID регуляторів
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/pid_mgr.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PID_Manager_Init(PID_Manager_t* mgr,
                                 PID_Controller_t* pid1,
                                 PID_Controller_t* pid2,
                                 PID_Controller_t* pid3)
{
    if (mgr == NULL || pid1 == NULL || pid2 == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    mgr->pid1 = pid1;
    mgr->pid2 = pid2;
    mgr->pid3 = pid3;  /* NULL = 2-контурний каскад */

    return SERVO_OK;
}

float PID_Manager_Update(PID_Manager_t* mgr,
                          float setpoint,
                          float input1,
                          float input2,
                          float input3,
                          uint32_t current_time_us)
{
    if (mgr == NULL || mgr->pid1 == NULL || mgr->pid2 == NULL) {
        return 0.0f;
    }

    PID_Compute(mgr->pid1, setpoint, input1, current_time_us);
    float output1 = PID_GetOutput(mgr->pid1);

    PID_Compute(mgr->pid2, output1, input2, current_time_us);
    float output2 = PID_GetOutput(mgr->pid2);

    if (mgr->pid3 != NULL) {
        PID_Compute(mgr->pid3, output2, input3, current_time_us);
        return PID_GetOutput(mgr->pid3);
    }

    return output2;
}

Servo_Status_t PID_Manager_ResetAll(PID_Manager_t* mgr)
{
    if (mgr == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    PID_Reset(mgr->pid1);
    PID_Reset(mgr->pid2);

    if (mgr->pid3 != NULL) {
        PID_Reset(mgr->pid3);
    }

    return SERVO_OK;
}
