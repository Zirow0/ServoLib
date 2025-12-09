/**
 * @file brake.c
 * @brief Реалізація інтерфейсу взаємодії з драйвером гальм
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/iface/brake.h"

/* Private variables ---------------------------------------------------------*/

/**
 * @brief Глобальний екземпляр драйвера гальм
 */
static Brake_Driver_t g_brake_driver;

/**
 * @brief Прапорець ініціалізації
 */
static bool g_is_initialized = false;

/* Exported functions --------------------------------------------------------*/

Servo_Status_t BrakeInterface_Init(const BrakeInterface_InitParams_t* params)
{
    if (params == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (g_is_initialized) {
        return SERVO_ERROR;  // Вже ініціалізовано
    }

    // Підготовка конфігурації драйвера
    Brake_Config_t config = {
        .gpio_port = params->gpio_port,
        .gpio_pin = params->gpio_pin,
        .active_high = params->active_high,
        .release_delay_ms = params->release_delay,
        .engage_timeout_ms = params->engage_timeout
    };

    // Ініціалізація драйвера
    Servo_Status_t status = Brake_Init(&g_brake_driver, &config);
    if (status != SERVO_OK) {
        return status;
    }

    g_is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t BrakeInterface_Deinit(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Перед деініціалізацією активуємо гальма (fail-safe)
    Brake_Engage(&g_brake_driver);

    g_is_initialized = false;

    return SERVO_OK;
}

Servo_Status_t BrakeInterface_Release(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_Release(&g_brake_driver);
}

Servo_Status_t BrakeInterface_Engage(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_Engage(&g_brake_driver);
}

Servo_Status_t BrakeInterface_Update(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_Update(&g_brake_driver);
}

Servo_Status_t BrakeInterface_NotifyActivity(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_NotifyActivity(&g_brake_driver);
}

Servo_Status_t BrakeInterface_SetMode(Brake_Mode_t mode)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_SetMode(&g_brake_driver, mode);
}

Brake_Mode_t BrakeInterface_GetMode(void)
{
    if (!g_is_initialized) {
        return BRAKE_MODE_AUTO;  // Значення за замовчуванням
    }

    return g_brake_driver.mode;
}

Brake_State_t BrakeInterface_GetState(void)
{
    if (!g_is_initialized) {
        return BRAKE_STATE_ENGAGED;  // Fail-safe
    }

    return Brake_GetState(&g_brake_driver);
}

Servo_Status_t BrakeInterface_GetStatus(BrakeInterface_Status_t* status)
{
    if (status == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Заповнення статусу
    status->state = g_brake_driver.state;
    status->mode = g_brake_driver.mode;
    status->is_initialized = g_is_initialized;

    // Час бездіяльності недоступний через інтерфейс
    // (внутрішня деталь драйвера)
    status->idle_time_ms = 0;

    return SERVO_OK;
}

bool BrakeInterface_IsEngaged(void)
{
    if (!g_is_initialized) {
        return true;  // Fail-safe
    }

    return Brake_IsEngaged(&g_brake_driver);
}

bool BrakeInterface_IsReleased(void)
{
    if (!g_is_initialized) {
        return false;  // Fail-safe
    }

    return (Brake_GetState(&g_brake_driver) == BRAKE_STATE_RELEASED);
}

Servo_Status_t BrakeInterface_EmergencyStop(void)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    return Brake_EmergencyEngage(&g_brake_driver);
}

Servo_Status_t BrakeInterface_ConfigureTimings(uint32_t release_delay_ms,
                                                uint32_t engage_timeout_ms)
{
    if (!g_is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Оновлення таймінгів
    g_brake_driver.config.release_delay_ms = release_delay_ms;
    g_brake_driver.config.engage_timeout_ms = engage_timeout_ms;

    return SERVO_OK;
}
