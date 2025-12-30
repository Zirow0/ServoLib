/**
 * @file derivative.c
 * @brief Реалізація обчислення похідної (швидкості)
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/util/derivative.h"
#include <math.h>

/* Exported functions --------------------------------------------------------*/

float Derivative_NormalizeAngleDelta(float delta_deg)
{
    // Приведення до діапазону [-180, 180]
    while (delta_deg > 180.0f) {
        delta_deg -= 360.0f;
    }
    while (delta_deg < -180.0f) {
        delta_deg += 360.0f;
    }
    return delta_deg;
}

float Derivative_CalculateVelocity(float current_pos, float last_pos,
                                   uint32_t current_time_us, uint32_t last_time_us)
{
    // Перевірка на нульовий час
    if (current_time_us <= last_time_us) {
        return 0.0f;
    }

    // Різниця часу в секундах
    float dt_s = (float)(current_time_us - last_time_us) / 1000000.0f;

    // Різниця позицій з нормалізацією переходу через 0/360
    float delta_pos = current_pos - last_pos;
    delta_pos = Derivative_NormalizeAngleDelta(delta_pos);

    // Швидкість (град/с)
    float velocity = delta_pos / dt_s;

    return velocity;
}
