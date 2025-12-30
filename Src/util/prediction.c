/**
 * @file prediction.c
 * @brief Реалізація передбачення позиції (екстраполяції)
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/util/prediction.h"
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
