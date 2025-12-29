/**
 * @file base.c
 * @brief Реалізація базового драйвера двигуна
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Компілювати цей файл для PWM або PWM_UDP драйверів */
#if defined(USE_MOTOR_PWM) || defined(USE_MOTOR_PWM_UDP)

#include "../../../Inc/drv/motor/base.h"
#include "../../../Inc/config.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Motor_Base_Init(Motor_Base_Data_t* base, const Motor_Params_t* params)
{
    if (base == NULL || params == NULL) {
        return SERVO_INVALID;
    }

    // Очищення структури
    memset(base, 0, sizeof(Motor_Base_Data_t));

    // Копіювання параметрів
    base->params = *params;

    // Ініціалізація захисту за замовчуванням
    base->prot.max_current = MOTOR_OVERCURRENT_LIMIT;
    base->prot.max_temperature = MOTOR_MAX_TEMPERATURE;
    base->prot.stall_timeout_ms = 1000;
    base->prot.protection_mask = MOTOR_PROTECTION_OVERCURRENT |
                                 MOTOR_PROTECTION_OVERHEAT;

    // Початковий стан
    base->state = MOTOR_STATE_IDLE;
    base->direction = MOTOR_DIR_FORWARD;
    base->current_power = 0.0f;
    base->is_initialized = true;
    base->emergency_flag = false;
    base->last_error = ERR_NONE;

    base->start_time_ms = HWD_Timer_GetMillis();
    base->last_update_ms = base->start_time_ms;

    return SERVO_OK;
}

Servo_Status_t Motor_Base_DeInit(Motor_Base_Data_t* base)
{
    if (base == NULL) {
        return SERVO_INVALID;
    }

    if (!base->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Очищення структури
    memset(base, 0, sizeof(Motor_Base_Data_t));

    return SERVO_OK;
}

Servo_Status_t Motor_Base_Update(Motor_Base_Data_t* base)
{
    if (base == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t delta_time = current_time - base->last_update_ms;

    // Оновлення часу роботи
    if (base->state == MOTOR_STATE_RUNNING) {
        base->stats.run_time_ms += delta_time;
    }

    base->last_update_ms = current_time;

    return SERVO_OK;
}

Servo_Status_t Motor_Base_SetPower(Motor_Base_Data_t* base, float power)
{
    if (base == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка аварійного режиму
    if (base->emergency_flag) {
        return SERVO_ERROR;
    }

    // Обмеження в межах параметрів
    float max_power = base->params.max_power;
    float min_power = base->params.min_power;

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
    if (base->params.invert_direction) {
        power = -power;
    }

    // Визначення напрямку
    if (power > 0.0f) {
        base->direction = MOTOR_DIR_FORWARD;
    } else if (power < 0.0f) {
        base->direction = MOTOR_DIR_BACKWARD;
    }

    base->current_power = power;

    // Оновлення стану
    if (power != 0.0f) {
        if (base->state != MOTOR_STATE_RUNNING) {
            base->stats.total_starts++;
        }
        base->state = MOTOR_STATE_RUNNING;
    } else {
        base->state = MOTOR_STATE_IDLE;
    }

    return SERVO_OK;
}

Servo_Status_t Motor_Base_CheckProtection(Motor_Base_Data_t* base,
                                          uint32_t current,
                                          uint32_t temperature)
{
    if (base == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка струмового захисту
    if ((base->prot.protection_mask & MOTOR_PROTECTION_OVERCURRENT) &&
        (current > base->prot.max_current)) {
        base->last_error = ERR_MOTOR_OVERCURRENT;
        base->state = MOTOR_STATE_ERROR;
        base->stats.error_count++;
        return SERVO_ERROR;
    }

    // Перевірка температурного захисту
    if ((base->prot.protection_mask & MOTOR_PROTECTION_OVERHEAT) &&
        (temperature > base->prot.max_temperature)) {
        base->last_error = ERR_MOTOR_OVERHEAT;
        base->state = MOTOR_STATE_ERROR;
        base->stats.error_count++;
        return SERVO_ERROR;
    }

    return SERVO_OK;
}

Servo_Status_t Motor_Base_ClearError(Motor_Base_Data_t* base)
{
    if (base == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    base->last_error = ERR_NONE;
    base->emergency_flag = false;

    if (base->state == MOTOR_STATE_ERROR) {
        base->state = MOTOR_STATE_IDLE;
    }

    return SERVO_OK;
}

Servo_Status_t Motor_Base_SetState(Motor_Base_Data_t* base, Motor_State_t state)
{
    if (base == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    base->state = state;

    return SERVO_OK;
}

Servo_Status_t Motor_Base_GetStats(Motor_Base_Data_t* base, Motor_Stats_t* stats)
{
    if (base == NULL || stats == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    // Копіювання статистики
    stats->run_time_ms = base->stats.run_time_ms;
    stats->total_starts = base->stats.total_starts;
    stats->error_count = base->stats.error_count;
    stats->current_power = base->current_power;
    stats->current_draw = base->stats.current_draw;
    stats->state = base->state;
    stats->direction = base->direction;

    return SERVO_OK;
}

Servo_Status_t Motor_Base_ConfigProtection(Motor_Base_Data_t* base,
                                           const Motor_Protection_Config_t* prot)
{
    if (base == NULL || prot == NULL || !base->is_initialized) {
        return SERVO_INVALID;
    }

    base->prot = *prot;

    return SERVO_OK;
}

#endif /* USE_MOTOR_PWM || USE_MOTOR_PWM_UDP */
