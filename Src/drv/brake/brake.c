/**
 * @file brake.c
 * @brief Реалізація універсального інтерфейсу драйвера гальм
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "drv/brake/brake.h"
#include "hwd/hwd_timer.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Отримання поточного часу (мс)
 */
static uint32_t Brake_GetTimeMs(void)
{
    return HWD_Timer_GetMillis();
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Brake_Init(Brake_Interface_t* brake, const Brake_Params_t* params)
{
    if (brake == NULL || params == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (brake->hw.init == NULL || brake->hw.engage == NULL || brake->hw.release == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    memset(&brake->data, 0, sizeof(Brake_Data_t));
    brake->data.engage_time_ms = params->engage_time_ms;
    brake->data.release_time_ms = params->release_time_ms;
    brake->data.state = BRAKE_STATE_ENGAGED;

    Servo_Status_t status = brake->hw.init(brake->driver_data);
    if (status != SERVO_OK) return status;

    return brake->hw.engage(brake->driver_data);
}

Servo_Status_t Brake_Engage(Brake_Interface_t* brake)
{
    if (brake == NULL) return SERVO_ERROR_NULL_PTR;

    Servo_Status_t status = brake->hw.engage(brake->driver_data);
    if (status != SERVO_OK) return status;

    brake->data.state = BRAKE_STATE_ENGAGING;
    brake->data.transition_start_time_ms = Brake_GetTimeMs();

    return SERVO_OK;
}

Servo_Status_t Brake_Release(Brake_Interface_t* brake)
{
    if (brake == NULL) return SERVO_ERROR_NULL_PTR;

    Servo_Status_t status = brake->hw.release(brake->driver_data);
    if (status != SERVO_OK) return status;

    brake->data.state = BRAKE_STATE_RELEASING;
    brake->data.transition_start_time_ms = Brake_GetTimeMs();

    return SERVO_OK;
}

Servo_Status_t Brake_Update(Brake_Interface_t* brake)
{
    if (brake == NULL) return SERVO_ERROR_NULL_PTR;

    uint32_t current_time = Brake_GetTimeMs();
    uint32_t elapsed_time = current_time - brake->data.transition_start_time_ms;

    // Обробка перехідних станів
    switch (brake->data.state) {
        case BRAKE_STATE_ENGAGING:
            // Перевірка завершення переходу ENGAGING → ENGAGED
            if (elapsed_time >= brake->data.engage_time_ms) {
                brake->data.state = BRAKE_STATE_ENGAGED;
            }
            break;

        case BRAKE_STATE_RELEASING:
            // Перевірка завершення переходу RELEASING → RELEASED
            if (elapsed_time >= brake->data.release_time_ms) {
                brake->data.state = BRAKE_STATE_RELEASED;
            }
            break;

        case BRAKE_STATE_ENGAGED:
        case BRAKE_STATE_RELEASED:
            // Стабільні стани - нічого не робимо
            break;

        default:
            return SERVO_ERROR;
    }

    return SERVO_OK;
}

Brake_State_t Brake_GetState(const Brake_Interface_t* brake)
{
    if (brake == NULL) return BRAKE_STATE_ENGAGED;  /* fail-safe */
    return brake->data.state;
}

bool Brake_IsEngaged(const Brake_Interface_t* brake)
{
    return Brake_GetState(brake) == BRAKE_STATE_ENGAGED;
}

bool Brake_IsReleased(const Brake_Interface_t* brake)
{
    return Brake_GetState(brake) == BRAKE_STATE_RELEASED;
}
