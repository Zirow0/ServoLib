/**
 * @file pwm_udp.c
 * @brief Реалізація драйвера PWM двигуна для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію драйвера PWM двигуна для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Компілювати цей файл тільки для UDP емуляції */
#ifdef USE_MOTOR_PWM_UDP

#include "../../../Inc/drv/motor/pwm_udp.h"
#include "../../../Emulator/udp_client.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t PWM_Motor_UDP_SendCommand(PWM_Motor_UDP_Driver_t* driver);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PWM_Motor_UDP_Create(PWM_Motor_UDP_Driver_t* driver,
                                    const PWM_Motor_UDP_Config_t* config)
{
    if (!driver || !config) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Копіювання конфігурації
    driver->config = *config;
    
    // Ініціалізація параметрів
    driver->current_power = 0.0f;
    driver->cmd.power = 0.0f;
    driver->last_update_ms = 0;
    driver->last_cmd_ms = 0;
    driver->error_count = 0;
    driver->comm_error_count = 0;
    driver->initialized = false;
    driver->enabled = false;
    driver->direction_inverted = config->type == MOTOR_TYPE_DC_PWM ? config->interface_mode == PWM_MOTOR_INTERFACE_UDP : false;
    
    driver->initialized = true;
    
    return SERVO_OK;
}

Servo_Status_t PWM_Motor_UDP_Init(PWM_Motor_UDP_Driver_t* driver,
                                  const Motor_Params_t* params)
{
    if (!driver || !params || !driver->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Копіювання параметрів
    driver->params = *params;
    
    // Ініціалізація UDP клієнта, якщо ще не ініціалізований
    Servo_Status_t status = UDP_Client_Init();
    if (status != SERVO_OK) {
        printf("ERROR: Failed to initialize UDP client for motor driver\n");
        return status;
    }
    
    // Відправлення початкової команди (двигун зупинений)
    status = PWM_Motor_UDP_SendCommand(driver);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to send initial motor command\n");
        return status;
    }
    
    driver->enabled = true;
    printf("UDP Motor driver initialized\n");
    
    return SERVO_OK;
}

Servo_Status_t PWM_Motor_UDP_SetPower(PWM_Motor_UDP_Driver_t* driver, float power)
{
    if (!driver || !driver->initialized || !driver->enabled) {
        return SERVO_NOT_INIT;
    }
    
    // Обмеження потужності
    if (power > 100.0f) {
        power = 100.0f;
    } else if (power < -100.0f) {
        power = -100.0f;
    }
    
    // Інверсія напрямку, якщо потрібно
    if (driver->params.invert_direction) {
        power = -power;
    }
    
    driver->current_power = power;
    driver->cmd.power = power;
    
    Servo_Status_t status = PWM_Motor_UDP_SendCommand(driver);
    if (status != SERVO_OK) {
        driver->error_count++;
        printf("ERROR: Failed to send motor power command: %.2f\n", power);
        return status;
    }
    
    driver->last_cmd_ms = 0; // Буде оновлено в системі таймінгу
    
    return SERVO_OK;
}

Servo_Status_t PWM_Motor_UDP_GetState(PWM_Motor_UDP_Driver_t* driver,
                                      float* power, float* position, float* velocity)
{
    if (!driver || !power || !position || !velocity) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    if (!driver->initialized) {
        return SERVO_NOT_INIT;
    }
    
    *power = driver->current_power;
    *position = driver->state.position;
    *velocity = driver->state.velocity;
    
    return SERVO_OK;
}

Servo_Status_t PWM_Motor_UDP_Stop(PWM_Motor_UDP_Driver_t* driver)
{
    if (!driver || !driver->initialized) {
        return SERVO_NOT_INIT;
    }
    
    return PWM_Motor_UDP_SetPower(driver, 0.0f);
}

Servo_Status_t PWM_Motor_UDP_EmergencyStop(PWM_Motor_UDP_Driver_t* driver)
{
    if (!driver || !driver->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Встановлюємо нульову потужність
    driver->current_power = 0.0f;
    driver->cmd.power = 0.0f;
    
    Servo_Status_t status = PWM_Motor_UDP_SendCommand(driver);
    if (status != SERVO_OK) {
        driver->error_count++;
        printf("ERROR: Failed to send emergency stop command\n");
        return status;
    }
    
    return SERVO_OK;
}

Servo_Status_t PWM_Motor_UDP_Update(PWM_Motor_UDP_Driver_t* driver)
{
    if (!driver || !driver->initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!driver->enabled) {
        return SERVO_OK;
    }
    
    // Отримання стану двигуна з моделі
    float position = 0.0f;
    float velocity = 0.0f;
    float current = 0.0f;
    bool stalled = false;
    bool overcurrent = false;
    
    Servo_Status_t status = UDP_Client_GetMotorStatus(&position, &velocity, 
                                                      &current, &stalled, &overcurrent);
    if (status == SERVO_OK) {
        driver->state.position = position;
        driver->state.velocity = velocity;
        driver->state.current = current;
        driver->state.stalled = stalled;
        driver->state.overcurrent = overcurrent;
        driver->state.error_flags = 0;
        
        if (stalled) driver->state.error_flags |= 0x01;
        if (overcurrent) driver->state.error_flags |= 0x02;
    } else if (status == SERVO_TIMEOUT) {
        // Таймаут - це нормально при першому запуску
        driver->comm_error_count++;
    } else {
        // Інша помилка
        driver->error_count++;
        driver->comm_error_count++;
    }
    
    return SERVO_OK;
}

UDP_Motor_State_t PWM_Motor_UDP_GetLastState(const PWM_Motor_UDP_Driver_t* driver)
{
    UDP_Motor_State_t state = {0};
    
    if (!driver || !driver->initialized) {
        return state;
    }
    
    return driver->state;
}

Servo_Status_t PWM_Motor_UDP_ResetErrorCount(PWM_Motor_UDP_Driver_t* driver)
{
    if (!driver) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    driver->error_count = 0;
    driver->comm_error_count = 0;
    
    return SERVO_OK;
}

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t PWM_Motor_UDP_SendCommand(PWM_Motor_UDP_Driver_t* driver)
{
    if (!driver) {
        return SERVO_ERROR_NULL_PTR;
    }

    return UDP_Client_SendMotorCommand(driver->cmd.power);
}

#endif /* USE_MOTOR_PWM_UDP */