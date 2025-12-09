/**
 * @file calib.c
 * @brief Реалізація калібрування
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/calib.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Calib_Init(Calibration_Data_t* calib)
{
    if (calib == NULL) {
        return SERVO_INVALID;
    }

    memset(calib, 0, sizeof(Calibration_Data_t));
    calib->scale_factor = 1.0f;
    calib->is_calibrated = false;

    return SERVO_OK;
}

Servo_Status_t Calib_SetZero(Calibration_Data_t* calib, float current_position)
{
    if (calib == NULL) {
        return SERVO_INVALID;
    }

    calib->zero_position = current_position;
    calib->is_calibrated = true;

    return SERVO_OK;
}

Servo_Status_t Calib_SetScale(Calibration_Data_t* calib, float scale)
{
    if (calib == NULL || scale == 0.0f) {
        return SERVO_INVALID;
    }

    calib->scale_factor = scale;

    return SERVO_OK;
}

float Calib_ApplyOffset(const Calibration_Data_t* calib, float raw_position)
{
    if (calib == NULL || !calib->is_calibrated) {
        return raw_position;
    }

    return (raw_position - calib->zero_position) * calib->scale_factor;
}

bool Calib_IsCalibrated(const Calibration_Data_t* calib)
{
    if (calib == NULL) {
        return false;
    }

    return calib->is_calibrated;
}
