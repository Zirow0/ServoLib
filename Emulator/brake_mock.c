/**
 * @file brake_mock.c
 * @brief Мок-реалізація функцій гальм для емуляції
 * @author ServoCore Team
 * @date 2025
 * 
 * Цей файл містить заглушки для функцій гальм, необхідних для емуляції.
 * Він забезпечує сумісність з існуючим кодом без потреби у реальних драйверах.
 */

#include "../Inc/drv/brake/brake.h"
#include <string.h>

// Глобальний об'єкт гальм для емуляції
static Brake_Driver_t g_brake_driver_mock;
static bool g_brake_initialized = false;

Servo_Status_t Brake_Init(Brake_Driver_t* brake, const Brake_Config_t* config)
{
    if (brake == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Копіювання конфігурації
    brake->config = *config;
    
    // Ініціалізація стану
    brake->state = BRAKE_STATE_ENGAGED;  // Fail-safe: гальма активні замовчуванням
    brake->mode = BRAKE_MODE_AUTO;
    brake->last_activity_ms = 0;
    brake->release_time_ms = 0;
    brake->is_initialized = true;
    brake->pending_release = false;

    g_brake_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Brake_Release(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    brake->state = BRAKE_STATE_RELEASED;
    return SERVO_OK;
}

Servo_Status_t Brake_Engage(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    brake->state = BRAKE_STATE_ENGAGED;
    return SERVO_OK;
}

Servo_Status_t Brake_Update(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    return SERVO_OK;
}

Servo_Status_t Brake_NotifyActivity(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Оновлення часу останньої активності
    brake->last_activity_ms = 0; // В емуляції не використовуємо таймер

    return SERVO_OK;
}

Servo_Status_t Brake_SetMode(Brake_Driver_t* brake, Brake_Mode_t mode)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    brake->mode = mode;
    return SERVO_OK;
}

Brake_State_t Brake_GetState(const Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return BRAKE_STATE_ENGAGED;  // Fail-safe
    }

    return brake->state;
}

bool Brake_IsEngaged(const Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return true;  // Fail-safe
    }

    return (brake->state == BRAKE_STATE_ENGAGED);
}

Servo_Status_t Brake_EmergencyEngage(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    brake->state = BRAKE_STATE_ENGAGED;
    return SERVO_OK;
}