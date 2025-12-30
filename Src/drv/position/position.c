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
 * @brief Конвертація raw позиції в градуси
 */
static float RawToAngleDegrees(const Position_Sensor_Interface_t* sensor, uint32_t raw_position)
{
    // Обчислити максимальне значення для роздільної здатності
    uint32_t max_count = (1U << sensor->resolution_bits);

    // Конвертувати в градуси (0-360)
    float degrees = ((float)raw_position * 360.0f) / (float)max_count;

    // Нормалізувати діапазон [0, 360)
    while (degrees >= 360.0f) {
        degrees -= 360.0f;
    }
    while (degrees < 0.0f) {
        degrees += 360.0f;
    }

    return degrees;
}

/**
 * @brief Оновлення multi-turn tracking
 */
static void UpdateMultiTurn(Position_Sensor_Data_t* data, float current_angle)
{
    float angle_diff = current_angle - data->last_position_deg;

    // Виявлення переходу через 0/360
    if (angle_diff > 180.0f) {
        // Перехід від ~360 до ~0 (зворотний оберт)
        data->revolution_count--;
    } else if (angle_diff < -180.0f) {
        // Перехід від ~0 до ~360 (прямий оберт)
        data->revolution_count++;
    }

    // Обчислити абсолютну позицію
    data->absolute_position_deg = current_angle + (float)data->revolution_count * 360.0f;
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

    data->position_deg = 0.0f;
    data->velocity_deg_s = 0.0f;
    data->revolution_count = 0;
    data->absolute_position_deg = 0.0f;

    data->last_timestamp_us = HWD_Timer_GetMicros();
    data->last_position_deg = 0.0f;
    data->predicted_position_deg = 0.0f;
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

    // 2. Конвертувати raw → degrees (спільна логіка)
    float current_angle = RawToAngleDegrees(sensor, raw.raw_position);

    // 3. Обчислити velocity
    if (raw.has_velocity) {
        // Датчик надає готову velocity
        data->velocity_deg_s = raw.raw_velocity;
    } else {
        // Обчислити з позиції (util/derivative.c)
        data->velocity_deg_s = Derivative_CalculateVelocity(
            current_angle,
            data->last_position_deg,
            raw.timestamp_us,
            data->last_timestamp_us
        );
    }

    // 4. Multi-turn tracking
    if (sensor->capabilities & POSITION_CAP_MULTITURN) {
        UpdateMultiTurn(data, current_angle);
    } else {
        // Без multi-turn
        data->absolute_position_deg = current_angle;
    }

    // 5. Prediction (util/prediction.c)
    uint32_t current_time_us = HWD_Timer_GetMicros();
    data->predicted_position_deg = Prediction_GetCurrentPosition(
        current_angle,
        data->velocity_deg_s,
        raw.timestamp_us,
        current_time_us
    );
    data->prediction_timestamp_us = current_time_us;

    // 6. Оновити дані
    data->position_deg = current_angle;
    data->last_position_deg = current_angle;
    data->last_timestamp_us = raw.timestamp_us;

    data->stats.update_count++;
    data->stats.last_position_deg = current_angle;
    data->stats.last_velocity_deg_s = data->velocity_deg_s;

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

    *position_deg = data->position_deg;
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

    *velocity_deg_s = data->velocity_deg_s;
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

    *abs_position_deg = data->absolute_position_deg;
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

    // Оновити prediction на поточний момент
    uint32_t current_time_us = HWD_Timer_GetMicros();
    float predicted = Prediction_GetCurrentPosition(
        data->position_deg,
        data->velocity_deg_s,
        data->last_timestamp_us,
        current_time_us
    );

    *position_deg = predicted;
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

    // Встановити нову нульову позицію
    float offset = position_deg - data->position_deg;

    // Застосувати offset
    data->position_deg = position_deg;
    data->last_position_deg = position_deg;
    data->absolute_position_deg += offset;

    // Скинути multi-turn якщо потрібно
    if (position_deg == 0.0f) {
        data->revolution_count = 0;
        data->absolute_position_deg = 0.0f;
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
