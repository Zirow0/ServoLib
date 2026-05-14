/**
 * @file servo.c
 * @brief Реалізація головного контролера сервоприводу
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/servo.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t UpdateSensorData(Servo_Controller_t* servo)
{
    if (servo->sensor == NULL) {
        return SERVO_OK;
    }

    Position_Sensor_Update(servo->sensor);
    Position_Sensor_GetPosition(servo->sensor, &servo->state.position);
    Position_Sensor_GetVelocity(servo->sensor, &servo->state.velocity);

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Servo_Init(Servo_Controller_t* servo,
                          const Servo_Config_t* config,
                          Motor_Interface_t* motor)
{
    return Servo_InitFull(servo, config, motor, NULL, NULL, NULL);
}

Servo_Status_t Servo_InitFull(Servo_Controller_t* servo,
                               const Servo_Config_t* config,
                               Motor_Interface_t* motor,
                               Position_Sensor_Interface_t* sensor,
                               Brake_Interface_t* brake,
                               Current_Sensor_Interface_t* current)
{
    if (servo == NULL || config == NULL || motor == NULL) {
        return SERVO_INVALID;
    }

    memset(servo, 0, sizeof(Servo_Controller_t));

    servo->config  = *config;
    servo->motor   = motor;
    servo->sensor  = sensor;
    servo->brake   = brake;
    servo->current = current;

    Safety_Init(&servo->safety, &config->safety_config);
    Cascade_Init(&servo->cascade, &config->cascade_config, CASCADE_MODE_POS);
    Traj_Init(&servo->traj, &config->traj_params);

    uint32_t period_ms = Time_FreqToPeriod(config->update_frequency);
    Time_InitPeriodicTimer(&servo->update_timer, period_ms);

    servo->state.mode  = SERVO_MODE_IDLE;
    servo->state.state = SERVO_STATE_READY;
    servo->state.error = ERR_NONE;

    servo->enable_trajectory = true;
    servo->is_initialized    = true;

    return SERVO_OK;
}

Servo_Status_t Servo_Update(Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    if (!Time_IsElapsed(&servo->update_timer)) {
        return SERVO_OK;
    }

    UpdateSensorData(servo);

    float current_a = 0.0f;
    if (servo->current != NULL) {
        Current_Sensor_GetCurrent(servo->current, &current_a);
    }

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Update(servo->brake);
    }

    Servo_Status_t safety_status = Safety_Update(&servo->safety,
                                                  servo->state.position,
                                                  servo->state.velocity,
                                                  current_a);
    if (safety_status != SERVO_OK) {
        Servo_EmergencyStop(servo);
        return SERVO_ERROR;
    }

    /* Генератор траєкторій (лише в позиційному режимі) */
    if (servo->enable_trajectory && servo->state.mode == SERVO_MODE_POSITION) {
        if (!Traj_IsCompleted(&servo->traj)) {
            Traj_Compute(&servo->traj);
            servo->state.target_position = Traj_GetPosition(&servo->traj);
            servo->state.target_velocity = Traj_GetVelocity(&servo->traj);
        }
    }

    uint32_t now_us = Time_GetMicros();

    if (servo->state.mode == SERVO_MODE_POSITION) {
        servo->cascade.target_pos = servo->state.target_position;
        float power = Cascade_Compute(&servo->cascade,
                                       servo->state.position,
                                       servo->state.velocity,
                                       current_a,
                                       now_us);
        Motor_SetPower(servo->motor, power);
        servo->state.state = SERVO_STATE_RUNNING;

    } else if (servo->state.mode == SERVO_MODE_VELOCITY) {
        servo->cascade.target_vel = servo->state.target_velocity;
        float power = Cascade_Compute(&servo->cascade,
                                       servo->state.position,
                                       servo->state.velocity,
                                       current_a,
                                       now_us);
        Motor_SetPower(servo->motor, power);
        servo->state.state = SERVO_STATE_RUNNING;

    } else if (servo->state.mode == SERVO_MODE_TORQUE) {
        float power = Cascade_Compute(&servo->cascade,
                                       servo->state.position,
                                       servo->state.velocity,
                                       current_a,
                                       now_us);
        Motor_SetPower(servo->motor, power);
        servo->state.state = SERVO_STATE_RUNNING;

    } else {
        Motor_Stop(servo->motor);
        servo->state.state = SERVO_STATE_READY;
    }

    Safety_WatchdogKick(&servo->safety);

    return SERVO_OK;
}

Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position_rad)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    if (!Safety_CheckPosition(&servo->safety, position_rad)) {
        servo->state.error = ERR_POSITION_LIMIT;
        position_rad = Safety_ClampPosition(&servo->safety, position_rad);
    }

    servo->state.target_position = position_rad;
    servo->state.mode = SERVO_MODE_POSITION;

    Cascade_SetMode(&servo->cascade, CASCADE_MODE_POS);

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Release(servo->brake);
    }

    if (servo->enable_trajectory) {
        Traj_Start(&servo->traj, servo->state.position, position_rad);
    }

    return SERVO_OK;
}

Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity_rad_s)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->state.target_velocity = velocity_rad_s;
    servo->state.mode = SERVO_MODE_VELOCITY;

    Cascade_SetMode(&servo->cascade, CASCADE_MODE_VEL);

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Release(servo->brake);
    }

    return SERVO_OK;
}

Servo_Status_t Servo_SetTorque(Servo_Controller_t* servo, float current_a)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->cascade.target_current = current_a;
    servo->state.mode = SERVO_MODE_TORQUE;

    Cascade_SetMode(&servo->cascade, CASCADE_MODE_TRQ);

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Release(servo->brake);
    }

    return SERVO_OK;
}

Servo_Status_t Servo_Stop(Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->state.mode = SERVO_MODE_IDLE;
    servo->state.target_position = servo->state.position;
    servo->state.target_velocity = 0.0f;

    Motor_Stop(servo->motor);
    Traj_Stop(&servo->traj);
    Cascade_Reset(&servo->cascade);

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Engage(servo->brake);
    }

    return SERVO_OK;
}

Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->state.mode  = SERVO_MODE_ERROR;
    servo->state.state = SERVO_STATE_EMERGENCY;

    Motor_EmergencyStop(servo->motor);
    Traj_Stop(&servo->traj);

    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Engage(servo->brake);
    }

    return SERVO_OK;
}

float Servo_GetPosition(const Servo_Controller_t* servo)
{
    if (servo == NULL) {
        return 0.0f;
    }
    return servo->state.position;
}

float Servo_GetVelocity(const Servo_Controller_t* servo)
{
    if (servo == NULL) {
        return 0.0f;
    }
    return servo->state.velocity;
}

Servo_State_t Servo_GetState(const Servo_Controller_t* servo)
{
    if (servo == NULL) {
        return SERVO_STATE_UNINIT;
    }
    return servo->state.state;
}

Servo_Status_t Servo_CalibrateZero(Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    return SERVO_ERROR;
}

Servo_Status_t Servo_EnableTrajectory(Servo_Controller_t* servo, bool enable)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->enable_trajectory = enable;

    return SERVO_OK;
}

bool Servo_IsAtTarget(const Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return false;
    }

    if (servo->state.mode != SERVO_MODE_POSITION) {
        return false;
    }

    float error = fabsf(servo->state.target_position - servo->state.position);

    return error < POSITION_ERROR_THRESHOLD;
}
