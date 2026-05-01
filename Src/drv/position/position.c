/**
 * @file position.c
 * @brief Базова логіка датчика положення
 *
 * Обробка: конвертація raw→position, velocity, multi-turn.
 * Апаратна специфіка — в hardware callbacks (incremental_encoder.c, as5600.c).
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_POSITION

#include "../../../Inc/drv/position/position.h"
#include "../../../Inc/util/derivative.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <math.h>
#include <string.h>

/* Private functions ---------------------------------------------------------*/

static float NormalizeAngle(float angle_rad)
{
    float r = fmodf(angle_rad, TWO_PI);
    if (r < 0.0f) r += TWO_PI;
    return r;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Position_Sensor_Init(Position_Sensor_Interface_t* sensor)
{
    if (sensor == NULL) {
        return SERVO_INVALID;
    }

    memset(&sensor->data, 0, sizeof(Position_Sensor_Data_t));

    sensor->data.last_timestamp_us = HWD_Timer_GetMicros();
    sensor->data.last_error        = ERR_NONE;

    if (sensor->hw.init != NULL) {
        Servo_Status_t s = sensor->hw.init(sensor->driver_data);
        if (s != SERVO_OK) {
            sensor->data.last_error = ERR_SENSOR_INIT_FAILED;
            return s;
        }
    }

    sensor->data.is_initialized = true;
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_Update(Position_Sensor_Interface_t* sensor)
{
    if (sensor == NULL || sensor->hw.read_raw == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    Position_Raw_Data_t raw;
    Servo_Status_t s = sensor->hw.read_raw(sensor->driver_data, &raw);
    if (s != SERVO_OK) {
        data->last_error = ERR_SENSOR_READ_FAILED;
        return s;
    }

    if (!raw.valid) {
        data->last_error = ERR_SENSOR_INVALID_DATA;
        return SERVO_ERROR;
    }

    float abs_rad     = raw.angle_rad + data->angle_offset_rad;
    float current_rad = NormalizeAngle(abs_rad);

    /* Velocity */
    if (raw.has_velocity) {
        data->velocity_rad_s = raw.velocity_rad_s;
    } else {
        data->velocity_rad_s = Derivative_CalculateVelocityRad(
            current_rad,
            data->last_position_rad,
            raw.timestamp_us,
            data->last_timestamp_us
        );
    }

    data->absolute_position_rad = abs_rad;
    data->position_rad          = current_rad;
    data->last_position_rad     = current_rad;
    data->last_timestamp_us     = raw.timestamp_us;

    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetPosition(Position_Sensor_Interface_t* sensor,
                                           float* position_deg)
{
    if (sensor == NULL || position_deg == NULL) return SERVO_INVALID;
    if (!sensor->data.is_initialized)           return SERVO_NOT_INIT;

    *position_deg = RAD2DEG(sensor->data.position_rad);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetVelocity(Position_Sensor_Interface_t* sensor,
                                           float* velocity_deg_s)
{
    if (sensor == NULL || velocity_deg_s == NULL) return SERVO_INVALID;
    if (!sensor->data.is_initialized)             return SERVO_NOT_INIT;

    *velocity_deg_s = RAD2DEG(sensor->data.velocity_rad_s);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetAbsolutePosition(Position_Sensor_Interface_t* sensor,
                                                   float* abs_position_deg)
{
    if (sensor == NULL || abs_position_deg == NULL) return SERVO_INVALID;
    if (!sensor->data.is_initialized)               return SERVO_NOT_INIT;

    *abs_position_deg = RAD2DEG(sensor->data.absolute_position_rad);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_SetPosition(Position_Sensor_Interface_t* sensor,
                                           float position_deg)
{
    if (sensor == NULL)                return SERVO_INVALID;
    if (!sensor->data.is_initialized) return SERVO_NOT_INIT;

    float new_rad   = DEG2RAD(position_deg);
    /* Сирий кут з драйвера (без поточного зсуву) */
    float raw_angle = sensor->data.absolute_position_rad - sensor->data.angle_offset_rad;

    sensor->data.angle_offset_rad      = new_rad - raw_angle;
    sensor->data.absolute_position_rad = new_rad;
    sensor->data.position_rad          = NormalizeAngle(new_rad);
    sensor->data.last_position_rad     = sensor->data.position_rad;

    return SERVO_OK;
}

#endif /* USE_SENSOR_POSITION */
