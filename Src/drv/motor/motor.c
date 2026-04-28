/**
 * @file motor.c
 * @brief Реалізація базового драйвера DC двигуна
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_MOTOR_PWM

#include "../../../Inc/drv/motor/motor.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Обробка потужності: обмеження, rate limit, мертва зона, інверсія
 */
static float ProcessPower(Motor_Data_t* data, float power)
{
    /* Обмеження в межах max_power */
    float max = data->params.max_power;
    if      (power >  max) power =  max;
    else if (power < -max) power = -max;

    /* Rate limiting */
    float rate = data->params.max_power_rate;
    if (rate > 0.0f) {
        float delta = power - data->prev_power;
        if      (delta >  rate) power = data->prev_power + rate;
        else if (delta < -rate) power = data->prev_power - rate;
    }

    /* Мертва зона */
    float dead = data->params.min_power;
    if (power > -dead && power < dead) {
        power = 0.0f;
    }

    /* Інверсія напрямку */
    if (data->params.invert_direction) {
        power = -power;
    }

    data->prev_power = power;
    return power;
}

/**
 * @brief Оновлення стану та напрямку на основі потужності
 */
static void UpdateState(Motor_Data_t* data, float power)
{
    if (power > 0.0f) {
        data->direction = MOTOR_DIR_FORWARD;
        data->state     = MOTOR_STATE_RUNNING;
    } else if (power < 0.0f) {
        data->direction = MOTOR_DIR_BACKWARD;
        data->state     = MOTOR_STATE_RUNNING;
    } else {
        data->state = MOTOR_STATE_IDLE;
    }

    data->current_power = power;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Motor_Init(Motor_Interface_t* motor, const Motor_Params_t* params)
{
    if (motor == NULL || params == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    memset(data, 0, sizeof(Motor_Data_t));

    data->params        = *params;
    data->state         = MOTOR_STATE_IDLE;
    data->direction     = MOTOR_DIR_FORWARD;
    data->current_power = 0.0f;
    data->prev_power    = 0.0f;
    data->is_initialized = true;
    data->emergency_flag = false;
    data->last_error    = ERR_NONE;

    if (motor->hw.init != NULL) {
        Servo_Status_t status = motor->hw.init(motor->driver_data, params);
        if (status != SERVO_OK) {
            data->is_initialized = false;
            return status;
        }
    }

    return SERVO_OK;
}

Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, float power)
{
    if (motor == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    if (data->emergency_flag) {
        return SERVO_ERROR;
    }

    float processed = ProcessPower(data, power);

    if (motor->hw.set_power != NULL) {
        Servo_Status_t status = motor->hw.set_power(motor->driver_data, processed);
        if (status != SERVO_OK) {
            return status;
        }
    }

    UpdateState(data, processed);

    return SERVO_OK;
}

Servo_Status_t Motor_Stop(Motor_Interface_t* motor)
{
    if (motor == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    if (motor->hw.stop != NULL) {
        Servo_Status_t status = motor->hw.stop(motor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    data->current_power = 0.0f;
    data->prev_power    = 0.0f;
    data->state         = MOTOR_STATE_IDLE;

    return SERVO_OK;
}

Servo_Status_t Motor_EmergencyStop(Motor_Interface_t* motor)
{
    if (motor == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    /* Миттєва зупинка — без rate limit */
    if (motor->hw.stop != NULL) {
        motor->hw.stop(motor->driver_data);
    }

    data->current_power  = 0.0f;
    data->prev_power     = 0.0f;
    data->emergency_flag = true;
    data->state          = MOTOR_STATE_ERROR;

    return SERVO_OK;
}

#endif /* USE_MOTOR_PWM */
