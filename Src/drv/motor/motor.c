/**
 * @file motor.c
 * @brief Реалізація базового драйвера двигуна
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Компілювати цей файл для PWM драйвера */
#ifdef USE_MOTOR_PWM

#include "../../../Inc/drv/motor/motor.h"
#include "../../../Inc/config.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Обробка потужності (обмеження, інверсія, мертва зона)
 */
static float ProcessPower(Motor_Data_t* data, float power)
{
    // Обмеження в межах параметрів
    float max_power = data->params.max_power;
    float min_power = data->params.min_power;

    if (power > max_power) {
        power = max_power;
    } else if (power < -max_power) {
        power = -max_power;
    }

    // Мертва зона
    if (power > -min_power && power < min_power) {
        power = 0.0f;
    }

    // Інверсія напрямку, якщо потрібно
    if (data->params.invert_direction) {
        power = -power;
    }

    return power;
}

/**
 * @brief Оновлення напрямку та стану на основі потужності
 */
static void UpdateDirectionAndState(Motor_Data_t* data, float power)
{
    // Визначення напрямку
    if (power > 0.0f) {
        data->direction = MOTOR_DIR_FORWARD;
    } else if (power < 0.0f) {
        data->direction = MOTOR_DIR_BACKWARD;
    }

    // Оновлення стану
    if (power != 0.0f) {
        if (data->state != MOTOR_STATE_RUNNING) {
            data->stats.total_starts++;
        }
        data->state = MOTOR_STATE_RUNNING;
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

    // Очищення структури
    memset(data, 0, sizeof(Motor_Data_t));

    // Копіювання параметрів
    data->params = *params;

    // Початковий стан
    data->state = MOTOR_STATE_IDLE;
    data->direction = MOTOR_DIR_FORWARD;
    data->current_power = 0.0f;
    data->is_initialized = true;
    data->emergency_flag = false;
    data->last_error = ERR_NONE;

    data->start_time_ms = HWD_Timer_GetMillis();
    data->last_update_ms = data->start_time_ms;

    // Викликати hardware init callback
    if (motor->hw.init != NULL) {
        Servo_Status_t status = motor->hw.init(motor->driver_data, params);
        if (status != SERVO_OK) {
            data->is_initialized = false;
            return status;
        }
    }

    return SERVO_OK;
}

Servo_Status_t Motor_DeInit(Motor_Interface_t* motor)
{
    if (motor == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Зупинка перед деініціалізацією
    Motor_Stop(motor);

    // Викликати hardware deinit callback
    if (motor->hw.deinit != NULL) {
        Servo_Status_t status = motor->hw.deinit(motor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // Очищення структури
    memset(data, 0, sizeof(Motor_Data_t));

    return SERVO_OK;
}

Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, const Motor_Command_t* cmd)
{
    if (motor == NULL || cmd == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Перевірка аварійного режиму
    if (data->emergency_flag) {
        return SERVO_ERROR;
    }

    // Отримання потужності з команди (залежно від типу)
    float power = 0.0f;
    switch (cmd->type) {
        case MOTOR_TYPE_DC_PWM:
            power = cmd->data.dc.power;
            break;
        case MOTOR_TYPE_STEPPER:
        case MOTOR_TYPE_BLDC:
            // Для багатофазних моторів обробка інша
            // Поки що просто використовуємо першу фазу
            power = cmd->data.stepper.phase_a;
            break;
        default:
            return SERVO_INVALID;
    }

    // Обробка потужності (обмеження, інверсія, мертва зона)
    float processed_power = ProcessPower(data, power);

    // Викликати hardware set_power callback з обробленою потужністю
    if (motor->hw.set_power != NULL) {
        Servo_Status_t status = motor->hw.set_power(motor->driver_data, cmd, processed_power);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // Оновлення напрямку та стану
    UpdateDirectionAndState(data, processed_power);

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

    // Викликати hardware stop callback
    if (motor->hw.stop != NULL) {
        Servo_Status_t status = motor->hw.stop(motor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // Скидання базових даних
    data->current_power = 0.0f;
    data->state = MOTOR_STATE_IDLE;

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

    // Викликати hardware stop callback
    if (motor->hw.stop != NULL) {
        Servo_Status_t status = motor->hw.stop(motor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // Встановлення аварійного режиму
    data->current_power = 0.0f;
    data->emergency_flag = true;
    data->state = MOTOR_STATE_ERROR;

    return SERVO_OK;
}

Servo_Status_t Motor_GetState(Motor_Interface_t* motor, Motor_State_t* state)
{
    if (motor == NULL || state == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    *state = data->state;
    return SERVO_OK;
}

Servo_Status_t Motor_GetStats(Motor_Interface_t* motor, Motor_Stats_t* stats)
{
    if (motor == NULL || stats == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Копіювання статистики
    stats->run_time_ms = data->stats.run_time_ms;
    stats->total_starts = data->stats.total_starts;
    stats->error_count = data->stats.error_count;
    stats->current_power = data->current_power;
    stats->state = data->state;
    stats->direction = data->direction;

    return SERVO_OK;
}

Servo_Status_t Motor_Update(Motor_Interface_t* motor)
{
    if (motor == NULL) {
        return SERVO_INVALID;
    }

    Motor_Data_t* data = &motor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t delta_time = current_time - data->last_update_ms;

    // Оновлення часу роботи
    if (data->state == MOTOR_STATE_RUNNING) {
        data->stats.run_time_ms += delta_time;
    }

    data->last_update_ms = current_time;

    // Викликати hardware update callback
    if (motor->hw.update != NULL) {
        Servo_Status_t status = motor->hw.update(motor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    return SERVO_OK;
}

#endif /* USE_MOTOR_PWM */
