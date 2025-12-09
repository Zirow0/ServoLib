/**
 * @file traj.c
 * @brief Реалізація генератора траєкторій
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/traj.h"
#include "../../Inc/ctrl/time.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Трапецоїдальна траєкторія (лінійна з обмеженнями)
 */
static void ComputeTrapezoidalTrajectory(Trajectory_Generator_t* traj)
{
    float distance = traj->target_position - traj->start_position;
    float direction = (distance >= 0.0f) ? 1.0f : -1.0f;
    distance = fabsf(distance);

    float t = traj->elapsed_time;
    float v_max = traj->params.max_velocity;
    float a_max = traj->params.max_acceleration;

    // Час прискорення
    float t_accel = v_max / a_max;
    float d_accel = 0.5f * a_max * t_accel * t_accel;

    // Перевірка чи треба гальмувати
    if (distance < 2.0f * d_accel) {
        // Трикутна траєкторія (без фази постійної швидкості)
        t_accel = sqrtf(distance / a_max);
        v_max = a_max * t_accel;
        d_accel = distance / 2.0f;
    }

    float t_cruise = (distance - 2.0f * d_accel) / v_max;
    float t_total = 2.0f * t_accel + t_cruise;

    traj->total_time_ms = (uint32_t)(t_total * 1000.0f);

    if (t <= t_accel) {
        // Фаза прискорення
        traj->state = TRAJ_STATE_ACCEL;
        traj->current_position = traj->start_position + direction * 0.5f * a_max * t * t;
        traj->current_velocity = direction * a_max * t;
        traj->current_acceleration = direction * a_max;
    } else if (t <= t_accel + t_cruise) {
        // Фаза постійної швидкості
        traj->state = TRAJ_STATE_CRUISE;
        float t_cruise_elapsed = t - t_accel;
        traj->current_position = traj->start_position + direction * (d_accel + v_max * t_cruise_elapsed);
        traj->current_velocity = direction * v_max;
        traj->current_acceleration = 0.0f;
    } else if (t <= t_total) {
        // Фаза уповільнення
        traj->state = TRAJ_STATE_DECEL;
        // float t_decel = t - t_accel - t_cruise;  // UNUSED
        float remaining_time = t_total - t;
        traj->current_velocity = direction * a_max * remaining_time;
        traj->current_position = traj->target_position - direction * 0.5f * a_max * remaining_time * remaining_time;
        traj->current_acceleration = -direction * a_max;
    } else {
        // Завершено
        traj->state = TRAJ_STATE_COMPLETED;
        traj->current_position = traj->target_position;
        traj->current_velocity = 0.0f;
        traj->current_acceleration = 0.0f;
        traj->is_active = false;
    }
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Traj_Init(Trajectory_Generator_t* traj, const Trajectory_Params_t* params)
{
    if (traj == NULL || params == NULL) {
        return SERVO_INVALID;
    }

    memset(traj, 0, sizeof(Trajectory_Generator_t));

    traj->params = *params;
    traj->state = TRAJ_STATE_IDLE;
    traj->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Traj_Start(Trajectory_Generator_t* traj, float start, float target)
{
    if (traj == NULL || !traj->is_initialized) {
        return SERVO_INVALID;
    }

    traj->start_position = start;
    traj->target_position = target;
    traj->current_position = start;
    traj->current_velocity = 0.0f;
    traj->current_acceleration = 0.0f;
    traj->start_time_ms = Time_GetMillis();
    traj->elapsed_time = 0.0f;
    traj->is_active = true;
    traj->state = TRAJ_STATE_ACCEL;

    return SERVO_OK;
}

Servo_Status_t Traj_Compute(Trajectory_Generator_t* traj)
{
    if (traj == NULL || !traj->is_initialized || !traj->is_active) {
        return SERVO_INVALID;
    }

    uint32_t current_time = Time_GetMillis();
    traj->elapsed_time = (float)(current_time - traj->start_time_ms) / 1000.0f;

    switch (traj->params.type) {
        case TRAJ_TYPE_LINEAR:
        case TRAJ_TYPE_SCURVE:
            ComputeTrapezoidalTrajectory(traj);
            break;

        case TRAJ_TYPE_CUBIC:
        case TRAJ_TYPE_QUINTIC:
            // TODO: Реалізувати кубічну та квінтичну траєкторії
            ComputeTrapezoidalTrajectory(traj);
            break;

        default:
            return SERVO_ERROR;
    }

    return SERVO_OK;
}

float Traj_GetPosition(const Trajectory_Generator_t* traj)
{
    if (traj == NULL) {
        return 0.0f;
    }
    return traj->current_position;
}

float Traj_GetVelocity(const Trajectory_Generator_t* traj)
{
    if (traj == NULL) {
        return 0.0f;
    }
    return traj->current_velocity;
}

bool Traj_IsCompleted(const Trajectory_Generator_t* traj)
{
    if (traj == NULL || !traj->is_initialized) {
        return true;
    }

    return traj->state == TRAJ_STATE_COMPLETED || !traj->is_active;
}

Servo_Status_t Traj_Stop(Trajectory_Generator_t* traj)
{
    if (traj == NULL || !traj->is_initialized) {
        return SERVO_INVALID;
    }

    traj->is_active = false;
    traj->state = TRAJ_STATE_IDLE;
    traj->current_velocity = 0.0f;
    traj->current_acceleration = 0.0f;

    return SERVO_OK;
}
