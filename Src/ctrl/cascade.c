/**
 * @file cascade.c
 * @brief Реалізація каскадного PID регулятора з feedforward
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/cascade.h"
#include <math.h>

/* Private defines -----------------------------------------------------------*/
#define CASCADE_PI   3.14159265358979323846f
#define CASCADE_2PI  6.28318530717958647692f

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Обмеження значення в межах [min, max]
 */
static inline float CascadeClamp(float v, float mn, float mx)
{
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

/**
 * @brief Ініціалізація одного PID контуру з Cascade_PID_Params_t
 */
static Servo_Status_t InitLoop(PID_Controller_t* pid, const Cascade_PID_Params_t* p)
{
    PID_Params_t params = {
        .Kp            = p->kp,
        .Ki            = p->ki,
        .Kd            = p->kd,
        .out_min       = p->out_min,
        .out_max       = p->out_max,
        .i_limit       = p->i_limit,
        .enabled_terms = PID_ENABLE_P | PID_ENABLE_I | PID_ENABLE_D,
    };
    return PID_Init(pid, &params);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Cascade_Init(Cascade_Controller_t* casc,
                             const Cascade_Config_t* config,
                             Cascade_Mode_t mode)
{
    if (casc == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    casc->config = *config;
    casc->mode   = mode;

    casc->target_pos      = 0.0f;
    casc->target_vel      = 0.0f;
    casc->target_current  = 0.0f;
    casc->prev_power      = 0.0f;
    casc->prev_time_us    = 0u;
    casc->last_vel_sp     = 0.0f;
    casc->last_current_sp = 0.0f;
    casc->last_ff         = 0.0f;
    casc->last_power      = 0.0f;

    Servo_Status_t s;
    s = InitLoop(&casc->pos_pid, &config->pos); if (s != SERVO_OK) return s;
    s = InitLoop(&casc->vel_pid, &config->vel); if (s != SERVO_OK) return s;
    s = InitLoop(&casc->trq_pid, &config->trq); if (s != SERVO_OK) return s;

    return SERVO_OK;
}

float Cascade_Compute(Cascade_Controller_t* casc,
                      float theta,
                      float omega,
                      float current_a,
                      uint32_t time_us)
{
    if (casc == NULL) {
        return 0.0f;
    }

    float current_sp;

    switch (casc->mode) {

        case CASCADE_MODE_POS: {
            /* Помилка положення, нормалізована до [-π, π] */
            float err = casc->target_pos - theta;
            err = fmodf(err + CASCADE_PI, CASCADE_2PI);
            if (err < 0.0f) err += CASCADE_2PI;
            err -= CASCADE_PI;

            /* pos-PID: virtual setpoint = theta + err, щоб PID бачив правильну похибку
             * при D-on-measurement (D рахується від theta, незалежно від setpoint).
             * Обмежуємо вхід vel-контуру одразу на виході pos-контуру. */
            PID_Compute(&casc->pos_pid, theta + err, theta, time_us);
            float vel_sp = CascadeClamp(PID_GetOutput(&casc->pos_pid),
                                        casc->config.pos.out_min,
                                        casc->config.pos.out_max);
            casc->last_vel_sp = vel_sp;

            /* vel-PID: обмежуємо вхід trq-контуру одразу на виході vel-контуру */
            PID_Compute(&casc->vel_pid, vel_sp, omega, time_us);
            current_sp = CascadeClamp(PID_GetOutput(&casc->vel_pid),
                                      casc->config.vel.out_min,
                                      casc->config.vel.out_max);
            casc->last_current_sp = current_sp;
            break;
        }

        case CASCADE_MODE_VEL: {
            float vel_sp = CascadeClamp(casc->target_vel,
                                        casc->config.pos.out_min,
                                        casc->config.pos.out_max);
            casc->last_vel_sp = vel_sp;
            PID_Compute(&casc->vel_pid, vel_sp, omega, time_us);
            current_sp = CascadeClamp(PID_GetOutput(&casc->vel_pid),
                                      casc->config.vel.out_min,
                                      casc->config.vel.out_max);
            casc->last_current_sp = current_sp;
            break;
        }

        case CASCADE_MODE_TRQ:
        default: {
            current_sp = CascadeClamp(casc->target_current,
                                      casc->config.vel.out_min,
                                      casc->config.vel.out_max);
            casc->last_vel_sp    = 0.0f;
            casc->last_current_sp = current_sp;
            break;
        }
    }

    /* Feedforward (спрощена модель):
     *   ff_j * current_sp  — передбачає момент інерції (% / A)
     *   ff_b * omega        — компенсує в'язке тертя (% / (рад/с))
     * current_sp вже обмежений вище, тому внесок ff_j природно обмежений.
     * У режимі TRQ ff_b не застосовується: користувач задає струм напряму. */
    float ff = 0.0f;
    if (casc->config.ff_j != 0.0f || casc->config.ff_b != 0.0f) {
        ff = casc->config.ff_j * current_sp;
        if (casc->mode != CASCADE_MODE_TRQ) {
            ff += casc->config.ff_b * omega;
        }
    }

    /* trq-PID + FF. Фінальне обмеження — єдине місце де обрізається вихід
     * останнього каскаду перед поверненням. */
    casc->last_ff = ff;
    PID_Compute(&casc->trq_pid, current_sp, current_a, time_us);
    float power = CascadeClamp(PID_GetOutput(&casc->trq_pid) + ff,
                                casc->config.trq.out_min,
                                casc->config.trq.out_max);

    /* Slew rate limiter.
     * dt = час від попереднього кроку (рахується через prev_time_us,
     * а не з PID — last_time_us вже оновлено всередині PID_Compute). */
    if (casc->config.slew_rate > 0.0f && casc->prev_time_us != 0u) {
        uint32_t delta_us = time_us - casc->prev_time_us;
        if (delta_us > 0u && delta_us < 200000u) {
            float dt = (float)delta_us / 1000000.0f;
            float max_delta = casc->config.slew_rate * dt;
            power = casc->prev_power +
                    CascadeClamp(power - casc->prev_power, -max_delta, max_delta);
        }
    }

    casc->prev_time_us = time_us;
    casc->prev_power   = power;
    casc->last_power   = power;
    return power;
}

Servo_Status_t Cascade_Reset(Cascade_Controller_t* casc)
{
    if (casc == NULL) {
        return SERVO_INVALID;
    }

    PID_Reset(&casc->pos_pid);
    PID_Reset(&casc->vel_pid);
    PID_Reset(&casc->trq_pid);
    casc->prev_power      = 0.0f;
    casc->prev_time_us    = 0u;
    casc->last_vel_sp     = 0.0f;
    casc->last_current_sp = 0.0f;
    casc->last_ff         = 0.0f;
    casc->last_power      = 0.0f;

    return SERVO_OK;
}

Servo_Status_t Cascade_SetMode(Cascade_Controller_t* casc, Cascade_Mode_t mode)
{
    if (casc == NULL) {
        return SERVO_INVALID;
    }

    casc->mode = mode;
    return Cascade_Reset(casc);
}

Servo_Status_t Cascade_ApplyConfig(Cascade_Controller_t* casc,
                                    const Cascade_Config_t* config)
{
    if (casc == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    casc->config = *config;

    /* pos-PID: оновити коефіцієнти та ліміти без скидання інтегратора */
    PID_SetTunings(&casc->pos_pid,
                   config->pos.kp, config->pos.ki, config->pos.kd);
    PID_SetOutputLimits(&casc->pos_pid,
                        config->pos.out_min, config->pos.out_max);
    casc->pos_pid.params.i_limit = config->pos.i_limit;

    /* vel-PID */
    PID_SetTunings(&casc->vel_pid,
                   config->vel.kp, config->vel.ki, config->vel.kd);
    PID_SetOutputLimits(&casc->vel_pid,
                        config->vel.out_min, config->vel.out_max);
    casc->vel_pid.params.i_limit = config->vel.i_limit;

    /* trq-PID */
    PID_SetTunings(&casc->trq_pid,
                   config->trq.kp, config->trq.ki, config->trq.kd);
    PID_SetOutputLimits(&casc->trq_pid,
                        config->trq.out_min, config->trq.out_max);
    casc->trq_pid.params.i_limit = config->trq.i_limit;

    return SERVO_OK;
}
