/**
 * @file brake_udp.c
 * @brief Реалізація драйвера електронних гальм для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію драйвера електронних гальм для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

/* Includes ------------------------------------------------------------------*/
#include "brake_udp.h"
#include "../../Emulator/udp_client.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t Brake_UDP_SendCommand(Brake_UDP_Driver_t* brake);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Brake_UDP_Create(Brake_UDP_Driver_t* brake, 
                                const Brake_UDP_Config_t* config)
{
    if (!brake || !config) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Копіювання конфігурації
    brake->config = *config;
    
    // Ініціалізація параметрів
    brake->cmd.engaged = true;  // Гальма активні за замовчуванням (fail-safe)
    brake->current_engaged = true;
    brake->last_update_ms = 0;
    brake->last_cmd_ms = 0;
    brake->error_count = 0;
    brake->comm_error_count = 0;
    brake->initialized = false;
    brake->enabled = false;
    brake->emergency_engaged = false;
    
    brake->initialized = true;
    
    return SERVO_OK;
}

Servo_Status_t Brake_UDP_Init(Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Ініціалізація UDP клієнта, якщо ще не ініціалізований
    Servo_Status_t status = UDP_Client_Init();
    if (status != SERVO_OK) {
        printf("ERROR: Failed to initialize UDP client for brake driver\n");
        return status;
    }
    
    // Відправлення початкової команди (гальма активні)
    status = Brake_UDP_SendCommand(brake);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to send initial brake command\n");
        return status;
    }
    
    brake->enabled = true;
    printf("UDP Brake driver initialized\n");
    
    return SERVO_OK;
}

Servo_Status_t Brake_UDP_Release(Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized || !brake->enabled) {
        return SERVO_NOT_INIT;
    }
    
    brake->cmd.engaged = false;
    brake->current_engaged = false;
    
    Servo_Status_t status = Brake_UDP_SendCommand(brake);
    if (status != SERVO_OK) {
        brake->error_count++;
        printf("ERROR: Failed to send brake release command\n");
        return status;
    }
    
    brake->last_cmd_ms = 0; // Буде оновлено в системі таймінгу
    
    printf("Brake released\n");
    return SERVO_OK;
}

Servo_Status_t Brake_UDP_Engage(Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized || !brake->enabled) {
        return SERVO_NOT_INIT;
    }
    
    brake->cmd.engaged = true;
    brake->current_engaged = true;
    
    Servo_Status_t status = Brake_UDP_SendCommand(brake);
    if (status != SERVO_OK) {
        brake->error_count++;
        printf("ERROR: Failed to send brake engage command\n");
        return status;
    }
    
    brake->last_cmd_ms = 0; // Буде оновлено в системі таймінгу
    
    printf("Brake engaged\n");
    return SERVO_OK;
}

Servo_Status_t Brake_UDP_EmergencyEngage(Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized) {
        return SERVO_NOT_INIT;
    }
    
    brake->cmd.engaged = true;
    brake->current_engaged = true;
    brake->emergency_engaged = true;
    
    Servo_Status_t status = Brake_UDP_SendCommand(brake);
    if (status != SERVO_OK) {
        brake->error_count++;
        printf("ERROR: Failed to send emergency brake engage command\n");
        return status;
    }
    
    printf("Emergency brake engaged\n");
    return SERVO_OK;
}

Servo_Status_t Brake_UDP_Update(Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!brake->enabled) {
        return SERVO_OK;
    }
    
    // Отримання стану гальм з моделі
    bool engaged = false;
    bool ready = false;
    
    Servo_Status_t status = UDP_Client_GetBrakeStatus(&engaged, &ready);
    if (status == SERVO_OK) {
        brake->state.engaged = engaged;
        brake->state.ready = ready;
        brake->current_engaged = engaged;
    } else if (status == SERVO_TIMEOUT) {
        // Таймаут - це нормально при першому запуску
        brake->comm_error_count++;
    } else {
        // Інша помилка
        brake->error_count++;
        brake->comm_error_count++;
    }
    
    return SERVO_OK;
}

bool Brake_UDP_IsEngaged(const Brake_UDP_Driver_t* brake)
{
    if (!brake || !brake->initialized) {
        return true; // Повертаємо true (гальма активні) для безпеки
    }
    
    return brake->current_engaged;
}

Servo_Status_t Brake_UDP_ResetErrorCount(Brake_UDP_Driver_t* brake)
{
    if (!brake) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    brake->error_count = 0;
    brake->comm_error_count = 0;
    
    return SERVO_OK;
}

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t Brake_UDP_SendCommand(Brake_UDP_Driver_t* brake)
{
    if (!brake) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    return UDP_Client_SendBrakeCommand(brake->cmd.engaged);
}