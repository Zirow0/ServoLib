/**
 * @file pid.c
 * @brief Реалізація PID регулятора
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/pid.h"
#include <string.h>

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

    // Очищення структури
    memset(pid, 0, sizeof(PID_Controller_t));

    // Копіювання параметрів
    pid->params = *params;

    // Встановлення прапорця
    pid->is_initialized = true;

    // Скидання внутрішнього стану
    PID_Reset(pid);

    return SERVO_OK;
}

Servo_Status_t PID_SetTunings(PID_Controller_t* pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    if (Kp < 0.0f || Ki < 0.0f || Kd < 0.0f) {
        return SERVO_INVALID;
    }

    // Зберігаємо оригінальні значення
    // Масштабування виконується при обчисленні в PID_Compute
    pid->params.Kp = Kp;
    pid->params.Ki = Ki;
    pid->params.Kd = Kd;

    return SERVO_OK;
}

Servo_Status_t PID_SetOutputLimits(PID_Controller_t* pid, float min, float max)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
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
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    // Обчислення дельти часу
    uint32_t delta_us = current_time_us - pid->last_time_us;
    float dt = delta_us / 1000000.0f;  // Конвертація мкс → секунди

    // Збереження поточного часу для наступної ітерації
    pid->last_time_us = current_time_us;

    pid->input = input;

    // Обчислення помилки
    float error = setpoint - input;

    // Пропорційна складова
    if (pid->params.enabled_terms & PID_ENABLE_P) {
        pid->p_term = pid->params.Kp * error;
    } else {
        pid->p_term = 0.0f;
    }

    // Інтегральна складова з масштабуванням за реальним часом
    if (pid->params.enabled_terms & PID_ENABLE_I) {
        float integral_increment = pid->params.Ki * dt * error;
        pid->integral += integral_increment;

        // Anti-windup: обмеження інтегральної складової
        pid->integral = Clamp(pid->integral, pid->params.out_min, pid->params.out_max);
        pid->i_term = pid->integral;
    } else {
        pid->i_term = 0.0f;
        pid->integral = 0.0f;  // Скидаємо інтегратор коли вимкнено
    }

    // Диференціальна складова (похідна від входу, а не від помилки)
    if (pid->params.enabled_terms & PID_ENABLE_D) {
        float d_input = input - pid->last_input;
        pid->d_term = -(pid->params.Kd / dt) * d_input;  // Мінус, бо рахуємо від входу
    } else {
        pid->d_term = 0.0f;
    }

    // Підсумковий вихід
    pid->output = pid->p_term + pid->i_term + pid->d_term;

    // Обмеження виходу
    pid->output = Clamp(pid->output, pid->params.out_min, pid->params.out_max);

    // Збереження для наступної ітерації
    pid->last_input = input;

    // Статистика
    pid->compute_count++;

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
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    pid->integral = 0.0f;
    pid->last_input = 0.0f;
    pid->output = 0.0f;
    pid->compute_count = 0;

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
