/**
 * @file pid.c
 * @brief Реалізація PID регулятора
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/pid.h"

/* Private defines -----------------------------------------------------------*/
#define PID_DT_MAX_S  0.1f   /**< Максимальний крок часу (захист від пауз/debug) */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Обмеження значення
 */
static inline float Clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PID_Init(PID_Controller_t* pid, const PID_Params_t* params)
{
    if (pid == NULL || params == NULL) {
        return SERVO_INVALID;
    }

    pid->params = *params;
    PID_Reset(pid);

    return SERVO_OK;
}

Servo_Status_t PID_SetTunings(PID_Controller_t* pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    pid->params.Kp = Kp;
    pid->params.Ki = Ki;
    pid->params.Kd = Kd;

    return SERVO_OK;
}

Servo_Status_t PID_SetOutputLimits(PID_Controller_t* pid, float min, float max)
{
    if (pid == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (min >= max) {
        return SERVO_INVALID;
    }

    pid->params.out_min = min;
    pid->params.out_max = max;

    // Обмеження поточного виходу
    pid->output = Clamp(pid->output, min, max);

    return SERVO_OK;
}


Servo_Status_t PID_Compute(PID_Controller_t* pid, float setpoint, float input, uint32_t current_time_us)
{
    if (pid == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Перший виклик після Init/Reset — ініціалізуємо стан без обчислення.
     * Захищає від D-spike (last_input=0) та некоректного dt. */
    if (pid->last_time_us == 0) {
        pid->last_time_us = current_time_us;
        pid->last_input   = input;
        return SERVO_OK;
    }

    uint32_t delta_us = current_time_us - pid->last_time_us;
    float dt = (float)delta_us / 1000000.0f;

    if (dt <= 0.0f) {
        return SERVO_OK;
    }

    if (dt > PID_DT_MAX_S) {
        dt = PID_DT_MAX_S;
    }

    pid->last_time_us = current_time_us;

    float error = setpoint - input;

    /* Пропорційна складова */
    pid->p_term = (pid->params.enabled_terms & PID_ENABLE_P)
                ? pid->params.Kp * error
                : 0.0f;

    /* Інтегральна складова (anti-windup: clamp з урахуванням P-term) */
    if (pid->params.enabled_terms & PID_ENABLE_I) {
        pid->integral += pid->params.Ki * dt * error;
        pid->integral  = Clamp(pid->integral,
                               pid->params.out_min - pid->p_term,
                               pid->params.out_max - pid->p_term);
        pid->i_term    = pid->integral;
    } else {
        pid->integral = 0.0f;
        pid->i_term   = 0.0f;
    }

    /* Диференціальна складова — derivative on measurement (без D-spike при зміні setpoint) */
    if (pid->params.enabled_terms & PID_ENABLE_D) {
        pid->d_term = -(pid->params.Kd / dt) * (input - pid->last_input);
    } else {
        pid->d_term = 0.0f;
    }

    pid->output    = Clamp(pid->p_term + pid->i_term + pid->d_term,
                           pid->params.out_min, pid->params.out_max);
    pid->last_input = input;

    return SERVO_OK;
}

float PID_GetOutput(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->output;
}

Servo_Status_t PID_Reset(PID_Controller_t* pid)
{
    if (pid == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    pid->integral     = 0.0f;
    pid->last_input   = 0.0f;
    pid->output       = 0.0f;
    pid->p_term       = 0.0f;
    pid->i_term       = 0.0f;
    pid->d_term       = 0.0f;
    pid->last_time_us = 0;  /* Наступний Compute — перший виклик */

    return SERVO_OK;
}

float PID_GetPTerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->p_term;
}

float PID_GetITerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->i_term;
}

float PID_GetDTerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->d_term;
}
