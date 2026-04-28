/**
 * @file safety.c
 * @brief Реалізація системи безпеки
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/safety.h"
#include "../../Inc/ctrl/time.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

static inline float ClampFloat(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Safety_Init(Safety_System_t* safety,
                           const Safety_Config_t* config)
{
    if (safety == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    memset(safety, 0, sizeof(Safety_System_t));

    safety->config = *config;
    safety->state.is_safe = true;
    safety->state.last_update_time = Time_GetMillis();
    safety->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Safety_Update(Safety_System_t* safety,
                             float position,
                             float velocity,
                             uint32_t current,
                             uint32_t temperature)
{
    if (safety == NULL || !safety->is_initialized) {
        return SERVO_INVALID;
    }

    uint32_t current_time = Time_GetMillis();
    safety->state.current_position = position;
    safety->state.current_velocity = velocity;
    safety->state.current_draw = current;
    safety->state.current_temperature = temperature;

    bool violation = false;

    // Перевірка положення
    if (safety->config.enable_position_limits) {
        if (position < safety->config.min_position || position > safety->config.max_position) {
            safety->state.position_violated = true;
            safety->state.last_violation = ERR_POSITION_LIMIT;
            violation = true;
        }
    }

    // Перевірка швидкості
    if (safety->config.enable_velocity_limit) {
        if (fabsf(velocity) > safety->config.max_velocity) {
            safety->state.velocity_violated = true;
            safety->state.last_violation = ERR_VELOCITY_LIMIT;
            violation = true;
        }
    }

    // Перевірка струму
    if (safety->config.enable_current_protection) {
        if (current > safety->config.max_current) {
            if (safety->state.overcurrent_start == 0) {
                safety->state.overcurrent_start = current_time;
            }

            uint32_t overcurrent_duration = current_time - safety->state.overcurrent_start;
            if (overcurrent_duration > safety->config.current_timeout_ms) {
                safety->state.current_violated = true;
                safety->state.last_violation = ERR_MOTOR_OVERCURRENT;
                violation = true;
            }
        } else {
            safety->state.overcurrent_start = 0;
        }
    }

    // Перевірка температури
    if (safety->config.enable_thermal_protection) {
        if (temperature > safety->config.max_temperature) {
            safety->state.thermal_violated = true;
            safety->state.last_violation = ERR_MOTOR_OVERHEAT;
            violation = true;
        }
    }

    // Перевірка watchdog
    if (safety->config.enable_watchdog) {
        uint32_t time_since_update = current_time - safety->state.last_update_time;
        if (time_since_update > safety->config.watchdog_timeout_ms) {
            safety->state.watchdog_violated = true;
            safety->state.last_violation = ERR_WATCHDOG;
            violation = true;
        }
    }

    safety->state.last_update_time = current_time;
    safety->state.is_safe = !violation;

    return violation ? SERVO_ERROR : SERVO_OK;
}

bool Safety_CheckPosition(const Safety_System_t* safety, float position)
{
    if (safety == NULL || !safety->is_initialized) {
        return false;
    }

    if (!safety->config.enable_position_limits) {
        return true;
    }

    return (position >= safety->config.min_position) &&
           (position <= safety->config.max_position);
}

bool Safety_CheckVelocity(const Safety_System_t* safety, float velocity)
{
    if (safety == NULL || !safety->is_initialized) {
        return false;
    }

    if (!safety->config.enable_velocity_limit) {
        return true;
    }

    return fabsf(velocity) <= safety->config.max_velocity;
}

float Safety_ClampPosition(const Safety_System_t* safety, float position)
{
    if (safety == NULL || !safety->is_initialized) {
        return position;
    }

    if (!safety->config.enable_position_limits) {
        return position;
    }

    return ClampFloat(position, safety->config.min_position, safety->config.max_position);
}

float Safety_ClampVelocity(const Safety_System_t* safety, float velocity)
{
    if (safety == NULL || !safety->is_initialized) {
        return velocity;
    }

    if (!safety->config.enable_velocity_limit) {
        return velocity;
    }

    return ClampFloat(velocity, -safety->config.max_velocity, safety->config.max_velocity);
}

Servo_Status_t Safety_ClearViolations(Safety_System_t* safety)
{
    if (safety == NULL || !safety->is_initialized) {
        return SERVO_INVALID;
    }

    safety->state.position_violated = false;
    safety->state.velocity_violated = false;
    safety->state.current_violated = false;
    safety->state.watchdog_violated = false;
    safety->state.thermal_violated = false;
    safety->state.is_safe = true;
    safety->state.last_violation = ERR_NONE;
    safety->state.overcurrent_start = 0;

    return SERVO_OK;
}

bool Safety_IsSafe(const Safety_System_t* safety)
{
    if (safety == NULL || !safety->is_initialized) {
        return false;
    }

    return safety->state.is_safe;
}

Servo_Status_t Safety_WatchdogKick(Safety_System_t* safety)
{
    if (safety == NULL || !safety->is_initialized) {
        return SERVO_INVALID;
    }

    safety->state.last_update_time = Time_GetMillis();
    safety->state.watchdog_violated = false;

    return SERVO_OK;
}

Servo_Status_t Safety_EnableLimit(Safety_System_t* safety,
                                  Safety_LimitType_t limit_type,
                                  bool enable)
{
    if (safety == NULL || !safety->is_initialized) {
        return SERVO_INVALID;
    }

    if (limit_type & SAFETY_LIMIT_POSITION) {
        safety->config.enable_position_limits = enable;
    }

    if (limit_type & SAFETY_LIMIT_VELOCITY) {
        safety->config.enable_velocity_limit = enable;
    }

    if (limit_type & SAFETY_LIMIT_CURRENT) {
        safety->config.enable_current_protection = enable;
    }

    return SERVO_OK;
}
