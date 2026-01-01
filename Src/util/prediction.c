/**
 * @file prediction.c
 * @brief Реалізація передбачення позиції (екстраполяції)
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/util/prediction.h"
#include "../../Inc/core.h"
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Нормалізація кута до діапазону [0, 360)
 */
static float normalize_angle(float angle_deg)
{
    while (angle_deg >= 360.0f) {
        angle_deg -= 360.0f;
    }
    while (angle_deg < 0.0f) {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

/**
 * @brief Нормалізація кута до діапазону [0, 2π)
 */
static float normalize_angle_rad(float angle_rad)
{
    while (angle_rad >= TWO_PI) {
        angle_rad -= TWO_PI;
    }
    while (angle_rad < 0.0f) {
        angle_rad += TWO_PI;
    }
    return angle_rad;
}

/* Exported functions --------------------------------------------------------*/

float Prediction_LinearExtrapolate(float position_deg, float velocity_deg_s, uint32_t dt_us)
{
    // Конвертувати dt з мікросекунд у секунди
    float dt_s = (float)dt_us / 1000000.0f;

    // Обчислити зміну позиції
    float delta_position = velocity_deg_s * dt_s;

    // Нова позиція
    float predicted_position = position_deg + delta_position;

    // Нормалізувати до діапазону [0, 360)
    return normalize_angle(predicted_position);
}

float Prediction_GetCurrentPosition(float last_position_deg, float velocity_deg_s,
                                   uint32_t last_time_us, uint32_t current_time_us)
{
    // Перевірка на некоректний час
    if (current_time_us < last_time_us) {
        return last_position_deg;
    }

    // Обчислити dt
    uint32_t dt_us = current_time_us - last_time_us;

    // Екстраполяція
    return Prediction_LinearExtrapolate(last_position_deg, velocity_deg_s, dt_us);
}

float Prediction_LinearExtrapolateRad(float position_rad, float velocity_rad_s, uint32_t dt_us)
{
    // Конвертувати dt з мікросекунд у секунди
    float dt_s = (float)dt_us / 1000000.0f;

    // Обчислити зміну позиції
    float delta_position = velocity_rad_s * dt_s;

    // Нова позиція
    float predicted_position = position_rad + delta_position;

    // Нормалізувати до діапазону [0, 2π)
    return normalize_angle_rad(predicted_position);
}

float Prediction_GetCurrentPositionRad(float last_position_rad, float velocity_rad_s,
                                       uint32_t last_time_us, uint32_t current_time_us)
{
    // Перевірка на некоректний час
    if (current_time_us < last_time_us) {
        return last_position_rad;
    }

    // Обчислити dt
    uint32_t dt_us = current_time_us - last_time_us;

    // Екстраполяція
    return Prediction_LinearExtrapolateRad(last_position_rad, velocity_rad_s, dt_us);
}
