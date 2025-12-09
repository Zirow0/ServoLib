/**
 * @file motor.c
 * @brief Реалізація абстрактного інтерфейсу двигуна
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/iface/motor.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Перевірка валідності інтерфейсу
 */
static inline bool Motor_IsValid(const Motor_Interface_t* motor)
{
    return (motor != NULL) && (motor->init != NULL);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Motor_Init(Motor_Interface_t* motor, const Motor_Params_t* params)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->init == NULL) {
        return SERVO_ERROR;
    }

    return motor->init(motor, params);
}

Servo_Status_t Motor_DeInit(Motor_Interface_t* motor)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->deinit == NULL) {
        return SERVO_ERROR;
    }

    return motor->deinit(motor);
}

Servo_Status_t Motor_SetPower(Motor_Interface_t* motor, float power)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->set_power == NULL) {
        return SERVO_ERROR;
    }

    // Обмеження потужності в діапазоні [-100, 100]
    if (power > 100.0f) {
        power = 100.0f;
    } else if (power < -100.0f) {
        power = -100.0f;
    }

    return motor->set_power(motor, power);
}

Servo_Status_t Motor_Stop(Motor_Interface_t* motor)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->stop == NULL) {
        return SERVO_ERROR;
    }

    return motor->stop(motor);
}

Servo_Status_t Motor_EmergencyStop(Motor_Interface_t* motor)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->emergency_stop == NULL) {
        // Якщо немає окремої функції аварійної зупинки, використовуємо звичайну
        return Motor_Stop(motor);
    }

    return motor->emergency_stop(motor);
}

Servo_Status_t Motor_SetDirection(Motor_Interface_t* motor, Motor_Direction_t direction)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->set_direction == NULL) {
        return SERVO_ERROR;
    }

    return motor->set_direction(motor, direction);
}

Servo_Status_t Motor_GetState(Motor_Interface_t* motor, Motor_State_t* state)
{
    if (!Motor_IsValid(motor) || state == NULL) {
        return SERVO_INVALID;
    }

    if (motor->get_state == NULL) {
        return SERVO_ERROR;
    }

    return motor->get_state(motor, state);
}

Servo_Status_t Motor_GetStats(Motor_Interface_t* motor, Motor_Stats_t* stats)
{
    if (!Motor_IsValid(motor) || stats == NULL) {
        return SERVO_INVALID;
    }

    if (motor->get_stats == NULL) {
        // Якщо немає статистики, повертаємо нульову структуру
        memset(stats, 0, sizeof(Motor_Stats_t));
        return SERVO_OK;
    }

    return motor->get_stats(motor, stats);
}

Servo_Status_t Motor_Update(Motor_Interface_t* motor)
{
    if (!Motor_IsValid(motor)) {
        return SERVO_INVALID;
    }

    if (motor->update == NULL) {
        // Якщо немає функції оновлення, просто повертаємо OK
        return SERVO_OK;
    }

    return motor->update(motor);
}
