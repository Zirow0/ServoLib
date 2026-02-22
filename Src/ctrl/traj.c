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
 * @brief S-подібна траєкторія з обмеженням ривка (jerk)
 */
static void ComputeSCurveTrajectory(Trajectory_Generator_t* traj)
{
    float distance = traj->target_position - traj->start_position;
    float direction = (distance >= 0.0f) ? 1.0f : -1.0f;
    distance = fabsf(distance);

    float t = traj->elapsed_time;
    float v_max = traj->params.max_velocity;
    float a_max = traj->params.max_acceleration;
    float j_max = traj->params.max_jerk;

    // Час наростання прискорення (jerk phase)
    float t_jerk = a_max / j_max;

    // Відстань під час фази jerk (1/6 * j * t^3)
    float d_jerk = (1.0f / 6.0f) * j_max * t_jerk * t_jerk * t_jerk;

    // Час постійного прискорення (між jerk фазами)
    float t_accel_const = (v_max - a_max * t_jerk) / a_max;
    if (t_accel_const < 0.0f) {
        t_accel_const = 0.0f;
    }

    // Загальний час прискорення (2 фази jerk + постійне прискорення)
    float t_accel_total = 2.0f * t_jerk + t_accel_const;

    // Відстань під час прискорення
    float d_accel = 2.0f * d_jerk + 0.5f * a_max * t_accel_const * t_accel_const +
                    a_max * t_jerk * t_accel_const;

    // Перевірка чи вистачає відстані для повного розгону
    if (distance < 2.0f * d_accel) {
        // Короткий рух - треба зменшити максимальну швидкість
        // Спрощення: використовуємо трапецоїдальну траєкторію для коротких рухів
        float t_accel_short = sqrtf(distance / a_max);
        v_max = a_max * t_accel_short;
        d_accel = distance / 2.0f;
        t_jerk = 0.0f;  // Відключаємо jerk для коротких рухів
        t_accel_total = t_accel_short;
    }

    // Відстань під час постійної швидкості
    float d_cruise = distance - 2.0f * d_accel;
    float t_cruise = d_cruise / v_max;

    // Загальний час руху
    float t_total = 2.0f * t_accel_total + t_cruise;
    traj->total_time_ms = (uint32_t)(t_total * 1000.0f);

    // Фаза 1: Зростання прискорення (jerk > 0)
    if (t <= t_jerk) {
        traj->state = TRAJ_STATE_ACCEL;
        float jerk = direction * j_max;
        traj->current_acceleration = jerk * t;
        traj->current_velocity = direction * 0.5f * j_max * t * t;
        traj->current_position = traj->start_position + direction * (1.0f / 6.0f) * j_max * t * t * t;
    }
    // Фаза 2: Постійне прискорення
    else if (t <= t_jerk + t_accel_const) {
        traj->state = TRAJ_STATE_ACCEL;
        float t_phase = t - t_jerk;
        traj->current_acceleration = direction * a_max;
        traj->current_velocity = direction * (a_max * t_jerk + a_max * t_phase);
        traj->current_position = traj->start_position + direction *
                                (d_jerk + a_max * t_jerk * t_phase + 0.5f * a_max * t_phase * t_phase);
    }
    // Фаза 3: Зменшення прискорення (jerk < 0)
    else if (t <= t_accel_total) {
        traj->state = TRAJ_STATE_ACCEL;
        float t_phase = t - t_jerk - t_accel_const;
        float jerk = -direction * j_max;
        traj->current_acceleration = direction * (a_max - j_max * t_phase);
        traj->current_velocity = direction * (v_max - 0.5f * j_max * (t_jerk - t_phase) * (t_jerk - t_phase));
        traj->current_position = traj->start_position + direction *
                                (d_accel - d_jerk + (1.0f / 6.0f) * j_max * (t_jerk * t_jerk * t_jerk -
                                (t_jerk - t_phase) * (t_jerk - t_phase) * (t_jerk - t_phase)));
    }
    // Фаза 4: Постійна швидкість
    else if (t <= t_accel_total + t_cruise) {
        traj->state = TRAJ_STATE_CRUISE;
        float t_cruise_elapsed = t - t_accel_total;
        traj->current_acceleration = 0.0f;
        traj->current_velocity = direction * v_max;
        traj->current_position = traj->start_position + direction * (d_accel + v_max * t_cruise_elapsed);
    }
    // Фаза 5-7: Уповільнення (дзеркало фаз 1-3)
    else if (t <= t_total) {
        traj->state = TRAJ_STATE_DECEL;
        float remaining_time = t_total - t;

        // Фаза 5: Зростання уповільнення (jerk < 0)
        if (remaining_time > t_jerk + t_accel_const) {
            float t_phase = t_accel_total - remaining_time;
            float jerk = -direction * j_max;
            traj->current_acceleration = jerk * t_phase;
            traj->current_velocity = direction * 0.5f * j_max * t_phase * t_phase;
            traj->current_position = traj->target_position - direction * (1.0f / 6.0f) * j_max * t_phase * t_phase * t_phase;
        }
        // Фаза 6: Постійне уповільнення
        else if (remaining_time > t_jerk) {
            float t_phase = remaining_time - t_jerk;
            traj->current_acceleration = -direction * a_max;
            traj->current_velocity = direction * (a_max * t_jerk + a_max * t_phase);
            traj->current_position = traj->target_position - direction *
                                    (d_jerk + a_max * t_jerk * t_phase + 0.5f * a_max * t_phase * t_phase);
        }
        // Фаза 7: Зменшення уповільнення (jerk > 0)
        else {
            float t_phase = remaining_time;
            float jerk = direction * j_max;
            traj->current_acceleration = -direction * (a_max - j_max * (t_jerk - t_phase));
            traj->current_velocity = direction * 0.5f * j_max * t_phase * t_phase;
            traj->current_position = traj->target_position - direction * (1.0f / 6.0f) * j_max * t_phase * t_phase * t_phase;
        }
    }
    else {
        // Завершено
        traj->state = TRAJ_STATE_COMPLETED;
        traj->current_position = traj->target_position;
        traj->current_velocity = 0.0f;
        traj->current_acceleration = 0.0f;
        traj->is_active = false;
    }
}

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
            ComputeTrapezoidalTrajectory(traj);
            break;

        case TRAJ_TYPE_SCURVE:
            ComputeSCurveTrajectory(traj);
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
