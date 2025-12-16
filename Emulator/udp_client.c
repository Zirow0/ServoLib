/**
 * @file udp_client.c
 * @brief Реалізація UDP клієнта для емуляції сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить реалізацію функцій для UDP комунікації з математичною моделлю.
 */

/* Includes ------------------------------------------------------------------*/
#include "udp_client.h"
#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static bool client_initialized = false;

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t SendUDPMessage(UDP_MsgType_t msg_type, const void* data, uint16_t size);

/* Exported functions --------------------------------------------------------*/

Servo_Status_t UDP_Client_Init(void)
{
    Servo_Status_t status = HWD_UDP_Init(UDP_SERVER_IP, UDP_SERVER_PORT, UDP_CLIENT_PORT);
    if (status != SERVO_OK) {
        return status;
    }
    
    client_initialized = true;
    printf("UDP Client initialized successfully\n");
    return SERVO_OK;
}

Servo_Status_t UDP_Client_SendMotorCommand(float power)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    UDP_Motor_Cmd_t cmd = {
        .power = power,
        .target_position = 0.0f,  // В емуляції можливо не використовується
        .target_velocity = 0.0f   // В емуляції можливо не використовується
    };
    
    return SendUDPMessage(UDP_MSG_TYPE_MOTOR_CMD, &cmd, sizeof(cmd));
}

Servo_Status_t UDP_Client_GetMotorStatus(float* position, float* velocity, 
                                         float* current, bool* stalled, bool* overcurrent)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!position || !velocity || !current || !stalled || !overcurrent) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    UDP_Message_t msg;
    Servo_Status_t status = HWD_UDP_Receive(&msg, UDP_TIMEOUT_MS);
    if (status != SERVO_OK) {
        return status;
    }
    
    if (msg.msg_type != UDP_MSG_TYPE_MOTOR_STATE) {
        return SERVO_INVALID;
    }
    
    if (msg.payload_size < sizeof(UDP_Motor_State_t)) {
        return SERVO_ERROR;
    }
    
    UDP_Motor_State_t* state = (UDP_Motor_State_t*)msg.payload;
    
    *position = state->position;
    *velocity = state->velocity;
    *current = state->current;
    *stalled = state->stalled;
    *overcurrent = state->overcurrent;
    
    return SERVO_OK;
}

Servo_Status_t UDP_Client_SendBrakeCommand(bool engaged)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    UDP_Brake_Cmd_t cmd = {
        .engaged = engaged,
        .timestamp = 0  // В емуляції можливо не використовується
    };
    
    return SendUDPMessage(UDP_MSG_TYPE_BRAKE_CMD, &cmd, sizeof(cmd));
}

Servo_Status_t UDP_Client_GetBrakeStatus(bool* engaged, bool* ready)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!engaged || !ready) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    UDP_Message_t msg;
    Servo_Status_t status = HWD_UDP_Receive(&msg, UDP_TIMEOUT_MS);
    if (status != SERVO_OK) {
        return status;
    }
    
    if (msg.msg_type != UDP_MSG_TYPE_BRAKE_STATE) {
        return SERVO_INVALID;
    }
    
    if (msg.payload_size < sizeof(UDP_Brake_State_t)) {
        return SERVO_ERROR;
    }
    
    UDP_Brake_State_t* state = (UDP_Brake_State_t*)msg.payload;
    
    *engaged = state->engaged;
    *ready = state->ready;
    
    return SERVO_OK;
}

Servo_Status_t UDP_Client_RequestSensorData(void)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    // Відправляємо запит на отримання даних сенсора
    return SendUDPMessage(UDP_MSG_TYPE_SENSOR_CMD, NULL, 0);
}

Servo_Status_t UDP_Client_GetSensorData(float* angle, float* velocity, bool* connected)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    if (!angle || !velocity || !connected) {
        return SERVO_ERROR_NULL_PTR;
    }
    
    UDP_Message_t msg;
    Servo_Status_t status = HWD_UDP_Receive(&msg, UDP_TIMEOUT_MS);
    if (status != SERVO_OK) {
        return status;
    }
    
    if (msg.msg_type != UDP_MSG_TYPE_SENSOR_STATE) {
        return SERVO_INVALID;
    }
    
    if (msg.payload_size < sizeof(UDP_Sensor_State_t)) {
        return SERVO_ERROR;
    }
    
    UDP_Sensor_State_t* state = (UDP_Sensor_State_t*)msg.payload;
    
    *angle = state->angle;
    *velocity = state->velocity;
    *connected = state->connected;
    
    return SERVO_OK;
}

Servo_Status_t UDP_Client_DeInit(void)
{
    if (client_initialized) {
        Servo_Status_t status = HWD_UDP_DeInit();
        client_initialized = false;
        return status;
    }
    
    return SERVO_OK;
}

Servo_Status_t UDP_Client_Ping(void)
{
    if (!client_initialized) {
        return SERVO_NOT_INIT;
    }
    
    return HWD_UDP_Ping();
}

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t SendUDPMessage(UDP_MsgType_t msg_type, const void* data, uint16_t size)
{
    if (size > sizeof(((UDP_Message_t*)0)->payload)) {
        return SERVO_INVALID;
    }
    
    UDP_Message_t msg = {0};
    msg.msg_type = msg_type;
    msg.sequence_number = 0; // В емуляції можливо не використовується
    msg.payload_size = size;
    
    if (data && size > 0) {
        memcpy(msg.payload, data, size);
    }
    
    return HWD_UDP_Send(&msg);
}
