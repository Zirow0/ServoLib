/**
 * @file sensor.c
 * @brief Реалізація інтерфейсу датчика
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/iface/sensor.h"

/* Private functions ---------------------------------------------------------*/

static inline bool Sensor_IsValid(const Sensor_Interface_t* sensor)
{
    return (sensor != NULL) && (sensor->init != NULL);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Sensor_Init(Sensor_Interface_t* sensor, const Sensor_Params_t* params)
{
    if (!Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    if (sensor->init == NULL) {
        return SERVO_ERROR;
    }

    return sensor->init(sensor, params);
}

Servo_Status_t Sensor_DeInit(Sensor_Interface_t* sensor)
{
    if (!Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    if (sensor->deinit == NULL) {
        return SERVO_ERROR;
    }

    return sensor->deinit(sensor);
}

Servo_Status_t Sensor_ReadAngle(Sensor_Interface_t* sensor, float* angle)
{
    if (!Sensor_IsValid(sensor) || angle == NULL) {
        return SERVO_INVALID;
    }

    if (sensor->read_angle == NULL) {
        return SERVO_ERROR;
    }

    return sensor->read_angle(sensor, angle);
}

Servo_Status_t Sensor_ReadVelocity(Sensor_Interface_t* sensor, float* velocity)
{
    if (!Sensor_IsValid(sensor) || velocity == NULL) {
        return SERVO_INVALID;
    }

    if (sensor->read_velocity == NULL) {
        // Якщо немає функції читання швидкості, повертаємо 0
        *velocity = 0.0f;
        return SERVO_OK;
    }

    return sensor->read_velocity(sensor, velocity);
}

Servo_Status_t Sensor_Calibrate(Sensor_Interface_t* sensor)
{
    if (!Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    if (sensor->calibrate == NULL) {
        // Якщо немає функції калібрування, вважаємо що не потрібно
        return SERVO_OK;
    }

    return sensor->calibrate(sensor);
}

Servo_Status_t Sensor_SelfTest(Sensor_Interface_t* sensor)
{
    if (!Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    if (sensor->self_test == NULL) {
        // Якщо немає функції самотестування
        return SERVO_OK;
    }

    return sensor->self_test(sensor);
}

Servo_Status_t Sensor_GetState(Sensor_Interface_t* sensor, Sensor_State_t* state)
{
    if (!Sensor_IsValid(sensor) || state == NULL) {
        return SERVO_INVALID;
    }

    if (sensor->get_state == NULL) {
        *state = SENSOR_STATE_IDLE;
        return SERVO_OK;
    }

    return sensor->get_state(sensor, state);
}

Servo_Status_t Sensor_GetStats(Sensor_Interface_t* sensor, Sensor_Stats_t* stats)
{
    if (!Sensor_IsValid(sensor) || stats == NULL) {
        return SERVO_INVALID;
    }

    if (sensor->get_stats == NULL) {
        return SERVO_ERROR;
    }

    return sensor->get_stats(sensor, stats);
}
