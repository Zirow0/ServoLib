/**
 * @file servo.c
 * @brief Реалізація головного контролера сервоприводу
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/servo.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Оновлення стану з датчика положення
 */
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
    return Servo_InitFull(servo, config, motor, NULL, NULL);
}

Servo_Status_t Servo_InitFull(Servo_Controller_t* servo,
                               const Servo_Config_t* config,
                               Motor_Interface_t* motor,
                               Position_Sensor_Interface_t* sensor,
                               Brake_Interface_t* brake)
{
    if (servo == NULL || config == NULL || motor == NULL) {
        return SERVO_INVALID;
    }

    memset(servo, 0, sizeof(Servo_Controller_t));

    // Копіювання конфігурації
    servo->config = *config;
    servo->motor = motor;
    servo->sensor = sensor;
    servo->brake = brake;

    // Ініціалізація підсистем
    Safety_Init(&servo->safety, &config->safety_config);

    PID_Init(&servo->pid, &config->pid_params);

    Traj_Init(&servo->traj, &config->traj_params);

    // Ініціалізація таймера оновлення
    uint32_t period_ms = Time_FreqToPeriod(config->update_frequency);
    Time_InitPeriodicTimer(&servo->update_timer, period_ms);

    // Початковий стан
    servo->state.mode = SERVO_MODE_IDLE;
    servo->state.state = SERVO_STATE_READY;
    servo->state.error = ERR_NONE;

    servo->enable_trajectory = true;
    servo->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Servo_Update(Servo_Controller_t* servo)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка чи настав час оновлення
    if (!Time_IsElapsed(&servo->update_timer)) {
        return SERVO_OK;
    }

    // Оновлення даних з датчиків
    UpdateSensorData(servo);

    // Оновлення гальм (якщо увімкнені)
    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Update(servo->brake);
    }

    // Перевірка безпеки
    Servo_Status_t safety_status = Safety_Update(&servo->safety,
                                                  servo->state.position,
                                                  servo->state.velocity,
                                                  0.0f); // TODO: Реальний струм

    if (safety_status != SERVO_OK) {
        // Аварійна зупинка при порушенні безпеки
        Servo_EmergencyStop(servo);
        return SERVO_ERROR;
    }

    // Обробка траєкторії
    if (servo->enable_trajectory && servo->state.mode == SERVO_MODE_POSITION) {
        if (!Traj_IsCompleted(&servo->traj)) {
            Traj_Compute(&servo->traj);
            servo->state.target_position = Traj_GetPosition(&servo->traj);
            servo->state.target_velocity = Traj_GetVelocity(&servo->traj);
        }
    }

    // PID регулювання в позиційному режимі
    if (servo->state.mode == SERVO_MODE_POSITION) {
        // Отримання поточного часу для PID
        uint32_t current_time_us = Time_GetMicros();

        PID_Compute(&servo->pid,
                    servo->state.target_position,  // setpoint
                    servo->state.position,          // input
                    current_time_us);               // час

        float pid_output = PID_GetOutput(&servo->pid);

        // Обмеження виходу системою безпеки
        pid_output = Safety_ClampVelocity(&servo->safety, pid_output);

        // Керування мотором
        Motor_SetPower(servo->motor, pid_output);

        servo->state.state = SERVO_STATE_RUNNING;
    }
    // Швидкісний режим
    else if (servo->state.mode == SERVO_MODE_VELOCITY) {
        float velocity = servo->state.target_velocity;
        velocity = Safety_ClampVelocity(&servo->safety, velocity);

        // Пряме керування швидкістю
        Motor_SetPower(servo->motor, velocity);

        servo->state.state = SERVO_STATE_RUNNING;
    }
    // Режим очікування
    else if (servo->state.mode == SERVO_MODE_IDLE) {
        Motor_Stop(servo->motor);
        servo->state.state = SERVO_STATE_READY;
    }

    // Watchdog
    Safety_WatchdogKick(&servo->safety);

    return SERVO_OK;
}

Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка безпеки
    if (!Safety_CheckPosition(&servo->safety, position)) {
        servo->state.error = ERR_POSITION_LIMIT;
        position = Safety_ClampPosition(&servo->safety, position);
    }

    servo->state.target_position = position;
    servo->state.mode = SERVO_MODE_POSITION;

    // Відпустити гальма перед рухом
    if (servo->brake != NULL && servo->config.enable_brake) {
        Brake_Release(servo->brake);
    }

    // Запуск траєкторії якщо увімкнено
    if (servo->enable_trajectory) {
        Traj_Start(&servo->traj, servo->state.position, position);
    }

    return SERVO_OK;
}

Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity)
{
    if (servo == NULL || !servo->is_initialized) {
        return SERVO_INVALID;
    }

    servo->state.target_velocity = velocity;
    servo->state.mode = SERVO_MODE_VELOCITY;

    // Відпустити гальма перед рухом
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
    PID_Reset(&servo->pid);

    // Активувати гальма після зупинки
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

    servo->state.mode = SERVO_MODE_ERROR;
    servo->state.state = SERVO_STATE_EMERGENCY;

    Motor_EmergencyStop(servo->motor);
    Traj_Stop(&servo->traj);

    // Аварійна активація гальм
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
