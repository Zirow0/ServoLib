/**
 * @file sensor_udp.c
 * @brief Реалізація драйвера сенсора для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію драйвера сенсора для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

/* Includes ------------------------------------------------------------------*/
#include "sensor_udp.h"
#include "../../Emulator/udp_client.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t Sensor_UDP_SendRequest(Sensor_UDP_Driver_t* sensor);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Sensor_UDP_Create(Sensor_UDP_Driver_t* sensor)
{
    if (!sensor) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    // Ініціалізація параметрів
    sensor->last_angle = 0.0f;
    sensor->last_velocity = 0.0f;
    sensor->last_update_ms = 0;
    sensor->last_request_ms = 0;
    sensor->update_interval_ms = SENSOR_UDP_DEFAULT_UPDATE_MS;
    sensor->error_count = 0;
    sensor->comm_error_count = 0;
    sensor->initialized = false;
    sensor->enabled = false;
    sensor->connected = false;
    sensor->request_pending = false;
    
    sensor->initialized = true;
    
    return SERVO_OK;
}

Servo_Status_t Sensor_UDP_Init(Sensor_UDP_Driver_t* sensor)
{
    if (!sensor || !sensor->initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Ініціалізація UDP клієнта, якщо ще не ініціалізований
    Servo_Status_t status = UDP_Client_Init();
    if (status != SERVO_OK) {
        printf("ERROR: Failed to initialize UDP client for sensor driver\n");
        return status;
    }
    
    // Відправлення початкового запиту на дані
    status = Sensor_UDP_SendRequest(sensor);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to send initial sensor request\n");
        return status;
    }
    
    sensor->enabled = true;
    printf("UDP Sensor driver initialized\n");
    
    return SERVO_OK;
}

Servo_Status_t Sensor_UDP_ReadAngle(Sensor_UDP_Driver_t* sensor, float* angle)
{
    if (!sensor || !angle) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    if (!sensor->initialized || !sensor->enabled) {
        return SERVO_NOT_INIT;
    }
    
    // Отримання даних сенсора з моделі
    float sensor_angle = 0.0f;
    float velocity = 0.0f;
    bool connected = false;
    
    Servo_Status_t status = UDP_Client_GetSensorData(&sensor_angle, &velocity, &connected);
    if (status == SERVO_OK) {
        *angle = sensor_angle;
        sensor->last_angle = sensor_angle;
        sensor->last_velocity = velocity;
        sensor->connected = connected;
        sensor->request_pending = false;
    } else if (status == SERVO_TIMEOUT) {
        // Таймаут - використовуємо останні відомі значення
        *angle = sensor->last_angle;
        sensor->comm_error_count++;
    } else {
        // Інша помилка
        *angle = sensor->last_angle; // Повертаємо останнє відоме значення
        sensor->error_count++;
        sensor->comm_error_count++;
    }
    
    return status;
}

Servo_Status_t Sensor_UDP_GetVelocity(Sensor_UDP_Driver_t* sensor, float* velocity)
{
    if (!sensor || !velocity) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    if (!sensor->initialized || !sensor->enabled) {
        return SERVO_NOT_INIT;
    }
    
    // Отримання даних сенсора з моделі
    float angle = 0.0f;
    float sensor_velocity = 0.0f;
    bool connected = false;
    
    Servo_Status_t status = UDP_Client_GetSensorData(&angle, &sensor_velocity, &connected);
    if (status == SERVO_OK) {
        *velocity = sensor_velocity;
        sensor->last_angle = angle;
        sensor->last_velocity = sensor_velocity;
        sensor->connected = connected;
        sensor->request_pending = false;
    } else if (status == SERVO_TIMEOUT) {
        // Таймаут - використовуємо останні відомі значення
        *velocity = sensor->last_velocity;
        sensor->comm_error_count++;
    } else {
        // Інша помилка
        *velocity = sensor->last_velocity; // Повертаємо останнє відоме значення
        sensor->error_count++;
        sensor->comm_error_count++;
    }
    
    return status;
}

Servo_Status_t Sensor_UDP_Update(Sensor_UDP_Driver_t* sensor)
{
    if (!sensor || !sensor->initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!sensor->enabled) {
        return SERVO_OK;
    }
    
    // Періодично надсилаємо запит на отримання даних
    // (це може бути викликано частіше ніж отримання даних)
    Servo_Status_t status = Sensor_UDP_SendRequest(sensor);
    if (status != SERVO_OK) {
        sensor->error_count++;
    }
    
    return SERVO_OK;
}

bool Sensor_UDP_IsConnected(const Sensor_UDP_Driver_t* sensor)
{
    if (!sensor || !sensor->initialized) {
        return false;
    }
    
    return sensor->connected;
}

UDP_Sensor_State_t Sensor_UDP_GetLastState(const Sensor_UDP_Driver_t* sensor)
{
    UDP_Sensor_State_t state = {0};
    
    if (!sensor || !sensor->initialized) {
        return state;
    }
    
    state.angle = sensor->last_angle;
    state.velocity = sensor->last_velocity;
    state.connected = sensor->connected;
    state.error_flags = 0; // В емуляції можливо не використовується
    
    return state;
}

Servo_Status_t Sensor_UDP_ResetErrorCount(Sensor_UDP_Driver_t* sensor)
{
    if (!sensor) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    sensor->error_count = 0;
    sensor->comm_error_count = 0;
    
    return SERVO_OK;
}

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t Sensor_UDP_SendRequest(Sensor_UDP_Driver_t* sensor)
{
    if (!sensor) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    return UDP_Client_RequestSensorData();
}