/**
 * @file calib.h
 * @brief Калібрування сервоприводу
 * @author ServoCore Team
 * @date 2025
 */

#ifndef SERVOCORE_CTRL_CALIB_H
#define SERVOCORE_CTRL_CALIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

typedef struct {
    float zero_position;      /**< Нульове положення (град) */
    float scale_factor;       /**< Масштабний коефіцієнт */
    bool is_calibrated;       /**< Прапорець калібрування */
} Calibration_Data_t;

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Calib_Init(Calibration_Data_t* calib);
Servo_Status_t Calib_SetZero(Calibration_Data_t* calib, float current_position);
Servo_Status_t Calib_SetScale(Calibration_Data_t* calib, float scale);
float Calib_ApplyOffset(const Calibration_Data_t* calib, float raw_position);
bool Calib_IsCalibrated(const Calibration_Data_t* calib);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_CALIB_H */
