/**
 * @file position.c
 * @brief Реалізація базової логіки датчика положення
 * @author ServoCore Team
 * @date 2025
 *
 * Універсальна логіка: конвертація, velocity, multi-turn, prediction.
 * Апаратна специфіка винесена в hardware callbacks (aeat9922.c, as5600.c).
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_POSITION

#include "../../../Inc/drv/position/position.h"
#include "../../../Inc/util/derivative.h"
#include "../../../Inc/util/prediction.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Нормалізація кута в радіанах до діапазону [0, 2π)
 */
static float NormalizeAngleRadians(float angle_rad)
{
    while (angle_rad >= TWO_PI) {
        angle_rad -= TWO_PI;
    }
    while (angle_rad < 0.0f) {
        angle_rad += TWO_PI;
    }
    return angle_rad;
}

/**
 * @brief Оновлення multi-turn tracking (в радіанах)
 */
static void UpdateMultiTurn(Position_Sensor_Data_t* data, float current_angle_rad)
{
    float angle_diff = current_angle_rad - data->last_position_rad;

    // Виявлення переходу через 0/2π
    if (angle_diff > PI) {
        // Перехід від ~2π до ~0 (зворотний оберт)
        data->revolution_count--;
    } else if (angle_diff < -PI) {
        // Перехід від ~0 до ~2π (прямий оберт)
        data->revolution_count++;
    }

    // Обчислити абсолютну позицію
    data->absolute_position_rad = current_angle_rad + (float)data->revolution_count * TWO_PI;
}

/**
 * @brief Перевірка валідності сенсора
 */
static inline bool Position_Sensor_IsValid(const Position_Sensor_Interface_t* sensor)
{
    return (sensor != NULL) && (sensor->hw.read_raw != NULL);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Position_Sensor_Init(Position_Sensor_Interface_t* sensor,
                                   const Position_Params_t* params)
{
    if (sensor == NULL || params == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    // Очищення структури
    memset(data, 0, sizeof(Position_Sensor_Data_t));

    // Початкові значення
    data->is_initialized = false;
    data->is_calibrated = false;
    data->last_error = ERR_NONE;

    data->position_rad = 0.0f;
    data->velocity_rad_s = 0.0f;
    data->revolution_count = 0;
    data->absolute_position_rad = 0.0f;

    data->last_timestamp_us = HWD_Timer_GetMicros();
    data->last_position_rad = 0.0f;
    data->predicted_position_rad = 0.0f;
    data->prediction_timestamp_us = data->last_timestamp_us;

    // Викликати hardware init callback
    if (sensor->hw.init != NULL) {
        Servo_Status_t status = sensor->hw.init(sensor->driver_data, params);
        if (status != SERVO_OK) {
            data->last_error = ERR_SENSOR_INIT_FAILED;
            return status;
        }
    }

    data->is_initialized = true;
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_DeInit(Position_Sensor_Interface_t* sensor)
{
    if (!Position_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Викликати hardware deinit callback
    if (sensor->hw.deinit != NULL) {
        Servo_Status_t status = sensor->hw.deinit(sensor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // Очищення структури
    memset(data, 0, sizeof(Position_Sensor_Data_t));

    return SERVO_OK;
}

Servo_Status_t Position_Sensor_Update(Position_Sensor_Interface_t* sensor)
{
    if (!Position_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // 1. Викликати hardware read_raw callback
    Position_Raw_Data_t raw;
    memset(&raw, 0, sizeof(Position_Raw_Data_t));

    Servo_Status_t status = sensor->hw.read_raw(sensor->driver_data, &raw);
    if (status != SERVO_OK) {
        data->stats.error_count++;
        data->last_error = ERR_SENSOR_READ_FAILED;
        return status;
    }

    if (!raw.valid) {
        data->stats.error_count++;
        data->last_error = ERR_SENSOR_INVALID_DATA;
        return SERVO_ERROR;
    }

    // 2. Дані вже в радіанах! Драйвер виконав конвертацію raw → radians.
    float current_angle_rad = NormalizeAngleRadians(raw.angle_rad);

    // 3. Обчислити velocity
    if (raw.has_velocity) {
        // Датчик надає готову velocity (в рад/с)
        data->velocity_rad_s = raw.velocity_rad_s;
    } else {
        // Обчислити з позиції (util/derivative.c)
        data->velocity_rad_s = Derivative_CalculateVelocityRad(
            current_angle_rad,
            data->last_position_rad,
            raw.timestamp_us,
            data->last_timestamp_us
        );
    }

    // 4. Multi-turn tracking
    if (sensor->capabilities & POSITION_CAP_MULTITURN) {
        UpdateMultiTurn(data, current_angle_rad);
    } else {
        // Без multi-turn
        data->absolute_position_rad = current_angle_rad;
    }

    // 5. Prediction (util/prediction.c)
    uint32_t current_time_us = HWD_Timer_GetMicros();
    data->predicted_position_rad = Prediction_GetCurrentPositionRad(
        current_angle_rad,
        data->velocity_rad_s,
        raw.timestamp_us,
        current_time_us
    );
    data->prediction_timestamp_us = current_time_us;

    // 6. Оновити дані
    data->position_rad = current_angle_rad;
    data->last_position_rad = current_angle_rad;
    data->last_timestamp_us = raw.timestamp_us;

    data->stats.update_count++;
    data->stats.last_position_deg = RAD2DEG(current_angle_rad);
    data->stats.last_velocity_deg_s = RAD2DEG(data->velocity_rad_s);

    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetPosition(Position_Sensor_Interface_t* sensor,
                                          float* position_deg)
{
    if (!Position_Sensor_IsValid(sensor) || position_deg == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Конвертувати радіани → градуси
    *position_deg = RAD2DEG(data->position_rad);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetVelocity(Position_Sensor_Interface_t* sensor,
                                          float* velocity_deg_s)
{
    if (!Position_Sensor_IsValid(sensor) || velocity_deg_s == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Конвертувати радіани → градуси
    *velocity_deg_s = RAD2DEG(data->velocity_rad_s);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetAbsolutePosition(Position_Sensor_Interface_t* sensor,
                                                  float* abs_position_deg)
{
    if (!Position_Sensor_IsValid(sensor) || abs_position_deg == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Конвертувати радіани → градуси
    *abs_position_deg = RAD2DEG(data->absolute_position_rad);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetPredictedPosition(Position_Sensor_Interface_t* sensor,
                                                    float* position_deg)
{
    if (!Position_Sensor_IsValid(sensor) || position_deg == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Оновити prediction на поточний момент (в радіанах)
    uint32_t current_time_us = HWD_Timer_GetMicros();
    float predicted_rad = Prediction_GetCurrentPositionRad(
        data->position_rad,
        data->velocity_rad_s,
        data->last_timestamp_us,
        current_time_us
    );

    // Конвертувати радіани → градуси
    *position_deg = RAD2DEG(predicted_rad);
    return SERVO_OK;
}

Servo_Status_t Position_Sensor_SetPosition(Position_Sensor_Interface_t* sensor,
                                          float position_deg)
{
    if (!Position_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Конвертувати градуси → радіани
    float position_rad = DEG2RAD(position_deg);

    // Встановити нову нульову позицію
    float offset = position_rad - data->position_rad;

    // Застосувати offset
    data->position_rad = position_rad;
    data->last_position_rad = position_rad;
    data->absolute_position_rad += offset;

    // Скинути multi-turn якщо потрібно
    if (position_deg == 0.0f) {
        data->revolution_count = 0;
        data->absolute_position_rad = 0.0f;
    }

    return SERVO_OK;
}

Servo_Status_t Position_Sensor_Calibrate(Position_Sensor_Interface_t* sensor)
{
    if (!Position_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Викликати hardware calibrate callback (якщо є)
    if (sensor->hw.calibrate != NULL) {
        Servo_Status_t status = sensor->hw.calibrate(sensor->driver_data);
        if (status != SERVO_OK) {
            data->last_error = ERR_SENSOR_CALIB_FAILED;
            return status;
        }
    }

    data->is_calibrated = true;
    data->stats.is_calibrated = true;

    return SERVO_OK;
}

Servo_Status_t Position_Sensor_GetStats(Position_Sensor_Interface_t* sensor,
                                       Position_Stats_t* stats)
{
    if (!Position_Sensor_IsValid(sensor) || stats == NULL) {
        return SERVO_INVALID;
    }

    Position_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    // Копіювання статистики
    *stats = data->stats;

    return SERVO_OK;
}

#endif /* USE_SENSOR_POSITION */
