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

    // Ініціалізація за замовчуванням
    pid->mode = PID_MODE_MANUAL;
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

    float sample_time = pid->params.sample_time;

    pid->params.Kp = Kp;
    pid->params.Ki = Ki * sample_time;  // Масштабуємо для дискретного часу
    pid->params.Kd = Kd / sample_time;

    // Інвертуємо для зворотного напрямку
    if (pid->params.direction == PID_DIRECTION_REVERSE) {
        pid->params.Kp = -pid->params.Kp;
        pid->params.Ki = -pid->params.Ki;
        pid->params.Kd = -pid->params.Kd;
    }

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

    // Обмеження інтегральної складової (anti-windup)
    pid->integral = Clamp(pid->integral, min, max);

    return SERVO_OK;
}

Servo_Status_t PID_SetMode(PID_Controller_t* pid, PID_Mode_t mode)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    bool new_auto = (mode == PID_MODE_AUTOMATIC);
    bool was_manual = (pid->mode == PID_MODE_MANUAL);

    // Перехід з MANUAL в AUTOMATIC
    if (new_auto && was_manual) {
        // Ініціалізація для плавного переходу
        pid->integral = pid->output;
        pid->last_input = pid->input;

        // Обмеження інтегралу
        pid->integral = Clamp(pid->integral, pid->params.out_min, pid->params.out_max);
    }

    pid->mode = mode;

    return SERVO_OK;
}

Servo_Status_t PID_SetDirection(PID_Controller_t* pid, PID_Direction_t direction)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    if (direction != pid->params.direction) {
        // Інвертуємо коефіцієнти
        pid->params.Kp = -pid->params.Kp;
        pid->params.Ki = -pid->params.Ki;
        pid->params.Kd = -pid->params.Kd;

        pid->params.direction = direction;
    }

    return SERVO_OK;
}

Servo_Status_t PID_SetSetpoint(PID_Controller_t* pid, float setpoint)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    pid->setpoint = setpoint;

    return SERVO_OK;
}

Servo_Status_t PID_SetSampleTime(PID_Controller_t* pid, float sample_time)
{
    if (pid == NULL || !pid->is_initialized || sample_time <= 0.0f) {
        return SERVO_INVALID;
    }

    float ratio = sample_time / pid->params.sample_time;

    // Перерахунок коефіцієнтів
    pid->params.Ki *= ratio;
    pid->params.Kd /= ratio;
    pid->params.sample_time = sample_time;

    return SERVO_OK;
}

Servo_Status_t PID_Compute(PID_Controller_t* pid, float input)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    // Якщо режим ручний, нічого не робимо
    if (pid->mode == PID_MODE_MANUAL) {
        return SERVO_OK;
    }

    pid->input = input;

    // Обчислення помилки
    float error = pid->setpoint - input;

    // Пропорційна складова
    float p_term = pid->params.Kp * error;

    // Інтегральна складова
    pid->integral += pid->params.Ki * error;

    // Anti-windup: обмеження інтегральної складової
    pid->integral = Clamp(pid->integral, pid->params.out_min, pid->params.out_max);

    // Диференціальна складова (похідна від входу, а не від помилки, щоб уникнути стрибків)
    float d_input = input - pid->last_input;
    float d_term = -pid->params.Kd * d_input;  // Мінус, бо рахуємо від входу

    // Підсумковий вихід
    pid->output = p_term + pid->integral + d_term;

    // Обмеження виходу
    pid->output = Clamp(pid->output, pid->params.out_min, pid->params.out_max);

    // Збереження для наступної ітерації
    pid->last_input = input;
    pid->last_error = error;

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

float PID_GetError(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->setpoint - pid->input;
}

Servo_Status_t PID_Reset(PID_Controller_t* pid)
{
    if (pid == NULL || !pid->is_initialized) {
        return SERVO_INVALID;
    }

    pid->integral = 0.0f;
    pid->last_input = 0.0f;
    pid->last_error = 0.0f;
    pid->output = 0.0f;
    pid->compute_count = 0;

    return SERVO_OK;
}

float PID_GetPTerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    float error = pid->setpoint - pid->input;
    return pid->params.Kp * error;
}

float PID_GetITerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    return pid->integral;
}

float PID_GetDTerm(const PID_Controller_t* pid)
{
    if (pid == NULL) {
        return 0.0f;
    }
    float d_input = pid->input - pid->last_input;
    return -pid->params.Kd * d_input;
}
