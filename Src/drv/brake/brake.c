/**
 * @file brake.c
 * @brief Реалізація драйвера електронних гальм
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "drv/brake/brake.h"
#include "hwd/hwd_gpio.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Встановлення GPIO стану для гальм
 */
static void Brake_SetGPIO(Brake_Driver_t* brake, bool active)
{
    HWD_GPIO_PinState_t state;

    if (brake->config.active_high) {
        state = active ? HWD_GPIO_PIN_SET : HWD_GPIO_PIN_RESET;
    } else {
        state = active ? HWD_GPIO_PIN_RESET : HWD_GPIO_PIN_SET;
    }

    // Використання HAL напряму для простоти
    HAL_GPIO_WritePin((GPIO_TypeDef*)brake->config.gpio_port,
                      brake->config.gpio_pin,
                      (GPIO_PinState)state);
}

/**
 * @brief Отримання поточного часу (мс)
 */
static uint32_t Brake_GetTimeMs(void)
{
    return HAL_GetTick();
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Brake_Init(Brake_Driver_t* brake, const Brake_Config_t* config)
{
    if (brake == NULL || config == NULL || config->gpio_port == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Копіювання конфігурації
    memcpy(&brake->config, config, sizeof(Brake_Config_t));

    // Ініціалізація стану
    brake->state = BRAKE_STATE_ENGAGED;  // Fail-safe: гальма активні за замовчуванням
    brake->mode = BRAKE_MODE_AUTO;
    brake->last_activity_ms = Brake_GetTimeMs();
    brake->release_time_ms = 0;
    brake->pending_release = false;

    // Встановлення GPIO в активний стан (гальма блокують)
    Brake_SetGPIO(brake, true);

    brake->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Brake_Release(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Відпускання гальм
    brake->state = BRAKE_STATE_RELEASED;
    Brake_SetGPIO(brake, false);

    // Оновлення часу відпускання
    brake->release_time_ms = Brake_GetTimeMs();
    brake->last_activity_ms = brake->release_time_ms;
    brake->pending_release = false;

    return SERVO_OK;
}

Servo_Status_t Brake_Engage(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Активація гальм
    brake->state = BRAKE_STATE_ENGAGED;
    Brake_SetGPIO(brake, true);

    brake->pending_release = false;

    return SERVO_OK;
}

Servo_Status_t Brake_Update(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Якщо режим ручний - нічого не робимо
    if (brake->mode == BRAKE_MODE_MANUAL) {
        return SERVO_OK;
    }

    uint32_t current_time = Brake_GetTimeMs();

    // Автоматичний режим
    if (brake->state == BRAKE_STATE_RELEASED) {
        // Гальма відпущені - перевіряємо таймаут бездіяльності
        uint32_t idle_time = current_time - brake->last_activity_ms;

        if (idle_time >= brake->config.engage_timeout_ms) {
            // Таймаут вичерпано - активуємо гальма
            Brake_Engage(brake);
        }
    } else {
        // Гальма активні - перевіряємо чи є очікування відпускання
        if (brake->pending_release) {
            uint32_t wait_time = current_time - brake->last_activity_ms;

            if (wait_time >= brake->config.release_delay_ms) {
                // Затримка минула - відпускаємо гальма
                Brake_Release(brake);
            }
        }
    }

    return SERVO_OK;
}

Servo_Status_t Brake_NotifyActivity(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Оновлення часу останньої активності
    brake->last_activity_ms = Brake_GetTimeMs();

    // Якщо гальма активні і режим автоматичний - відпускаємо їх
    if (brake->state == BRAKE_STATE_ENGAGED && brake->mode == BRAKE_MODE_AUTO) {
        // Встановлюємо прапорець очікування
        brake->pending_release = true;
    }

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
    return Brake_GetState(brake) == BRAKE_STATE_ENGAGED;
}

Servo_Status_t Brake_EmergencyEngage(Brake_Driver_t* brake)
{
    if (brake == NULL || !brake->is_initialized) {
        return SERVO_ERROR_NULL_PTR;
    }

    // Миттєва активація незалежно від режиму
    brake->state = BRAKE_STATE_ENGAGED;
    Brake_SetGPIO(brake, true);
    brake->pending_release = false;

    return SERVO_OK;
}
