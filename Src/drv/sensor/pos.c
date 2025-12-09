/**
 * @file pos.c
 * @brief Реалізація універсальної обгортки датчика положення
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_POSITION

#include "../../../Inc/drv/sensor/pos.h"
#include "../../../Inc/util/math.h"
#include <string.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Калібрування значення кута
 */
static float CalibrateAngle(const Pos_Sensor_Handle_t* handle, float angle)
{
    if (!handle->config.enable_calibration) {
        return angle;
    }

    // Застосування зміщення
    angle -= handle->config.zero_offset;

    // Застосування масштабування
    angle *= handle->config.scale_factor;

    // Інвертування напрямку
    if (handle->config.invert_direction) {
        angle = -angle;
    }

    // Нормалізація кута
    angle = Math_NormalizeAngle(angle);

    return angle;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Pos_Sensor_Init(Pos_Sensor_Handle_t* handle,
                               const Pos_Sensor_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->sensor == NULL) {
        return SERVO_INVALID;
    }

    // Очищення структури
    memset(handle, 0, sizeof(Pos_Sensor_Handle_t));

    // Копіювання конфігурації
    handle->config = *config;

    // Встановлення значень за замовчуванням
    if (handle->config.scale_factor == 0.0f) {
        handle->config.scale_factor = 1.0f;
    }

    handle->data.data_valid = false;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_DeInit(Pos_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    memset(handle, 0, sizeof(Pos_Sensor_Handle_t));

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_Update(Pos_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (handle->config.sensor == NULL) {
        return SERVO_INVALID;
    }

    Servo_Status_t status;

    // Читання кута
    status = handle->config.sensor->read_angle(
        handle->config.sensor,
        &handle->data.raw_angle
    );

    if (status != SERVO_OK) {
        handle->data.error_count++;
        handle->data.data_valid = false;
        return status;
    }

    // Читання швидкості
    status = handle->config.sensor->read_velocity(
        handle->config.sensor,
        &handle->data.raw_velocity
    );

    if (status != SERVO_OK) {
        handle->data.error_count++;
        handle->data.data_valid = false;
        return status;
    }

    // Калібрування (без фільтрації)
    handle->data.calibrated_angle = CalibrateAngle(handle, handle->data.raw_angle);

    // Оновлення лічильників
    handle->data.read_count++;
    handle->data.data_valid = true;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_GetAngle(const Pos_Sensor_Handle_t* handle, float* angle)
{
    if (handle == NULL || angle == NULL) {
        return SERVO_INVALID;
    }

    if (!handle->data.data_valid) {
        return SERVO_ERROR;
    }

    *angle = handle->data.calibrated_angle;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_GetVelocity(const Pos_Sensor_Handle_t* handle, float* velocity)
{
    if (handle == NULL || velocity == NULL) {
        return SERVO_INVALID;
    }

    if (!handle->data.data_valid) {
        return SERVO_ERROR;
    }

    *velocity = handle->data.raw_velocity;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_GetRawData(const Pos_Sensor_Handle_t* handle,
                                     float* angle,
                                     float* velocity)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (!handle->data.data_valid) {
        return SERVO_ERROR;
    }

    if (angle != NULL) {
        *angle = handle->data.raw_angle;
    }

    if (velocity != NULL) {
        *velocity = handle->data.raw_velocity;
    }

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_SetZero(Pos_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (!handle->data.data_valid) {
        return SERVO_ERROR;
    }

    // Встановлення поточного кута як нульового зміщення
    handle->config.zero_offset = handle->data.raw_angle;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_SetOffset(Pos_Sensor_Handle_t* handle, float offset)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    handle->config.zero_offset = offset;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_SetScale(Pos_Sensor_Handle_t* handle, float scale)
{
    if (handle == NULL || scale == 0.0f) {
        return SERVO_INVALID;
    }

    handle->config.scale_factor = scale;

    return SERVO_OK;
}

Servo_Status_t Pos_Sensor_GetStats(const Pos_Sensor_Handle_t* handle,
                                   uint32_t* read_count,
                                   uint32_t* error_count)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (read_count != NULL) {
        *read_count = handle->data.read_count;
    }

    if (error_count != NULL) {
        *error_count = handle->data.error_count;
    }

    return SERVO_OK;
}

bool Pos_Sensor_IsDataValid(const Pos_Sensor_Handle_t* handle)
{
    if (handle == NULL) {
        return false;
    }

    return handle->data.data_valid;
}

#endif /* USE_SENSOR_POSITION */
