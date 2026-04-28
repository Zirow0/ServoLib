/**
 * @file current.c
 * @brief Реалізація базової логіки датчика струму
 * @author ServoCore Team
 * @date 2025
 *
 * Спільна логіка: EMA фільтрація, корекція нуля, пік, перевантаження,
 * програмне калібрування через усереднення read_raw.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_CURRENT

#include "drv/current/current.h"
#include "hwd/hwd_timer.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

#define CURRENT_CALIB_DURATION_MS   50U   /**< Тривалість калібрування (мс) */
#define CURRENT_CALIB_MIN_SAMPLES   10U   /**< Мінімум зразків для калібрування */

/* Private functions ---------------------------------------------------------*/

static inline bool Current_Sensor_IsValid(const Current_Sensor_Interface_t* sensor)
{
    return (sensor != NULL) && (sensor->hw.read_raw != NULL);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Current_Sensor_Init(Current_Sensor_Interface_t* sensor,
                                    const Current_Params_t* params)
{
    if (sensor == NULL || params == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (sensor->hw.read_raw == NULL) {
        return SERVO_INVALID;
    }

    /* Очищення даних */
    memset(&sensor->data, 0, sizeof(Current_Sensor_Data_t));

    /* Збереження параметрів обробки */
    sensor->data.ema_alpha               = params->ema_alpha;
    sensor->data.overcurrent_threshold_a = params->overcurrent_threshold_a;

    /* Ініціалізація апаратури */
    if (sensor->hw.init != NULL) {
        Servo_Status_t status = sensor->hw.init(sensor->driver_data, params);
        if (status != SERVO_OK) {
            sensor->data.last_error = ERR_SENSOR_INIT_FAILED;
            return status;
        }
    }

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_Update(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) return SERVO_INVALID;

    Current_Sensor_Data_t* data = &sensor->data;

    /* 1. Зчитати миттєвий струм з драйвера */
    Current_Raw_Data_t raw;
    memset(&raw, 0, sizeof(Current_Raw_Data_t));

    Servo_Status_t status = sensor->hw.read_raw(sensor->driver_data, &raw);
    if (status != SERVO_OK) return status;

    if (!raw.valid) return SERVO_ERROR;

    /* 2. Корекція нульового зміщення */
    float current = raw.current_a - data->zero_offset_a;

    /* 3. EMA фільтрація: y[n] = α·x[n] + (1-α)·y[n-1] */
    data->filtered_current_a = data->ema_alpha * current
                              + (1.0f - data->ema_alpha) * data->filtered_current_a;

    /* 4. Відстеження абсолютного піку */
    float abs_current = (current < 0.0f) ? -current : current;
    if (abs_current > data->peak_current_a) {
        data->peak_current_a = abs_current;
    }

    /* 5. Виявлення перевантаження */
    if (data->overcurrent_threshold_a > 0.0f &&
        abs_current > data->overcurrent_threshold_a)
    {
        data->overcurrent_flag = true;
    }

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_Calibrate(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) return SERVO_INVALID;

    Current_Sensor_Data_t* data = &sensor->data;

    /* Програмне калібрування: усереднення read_raw за CURRENT_CALIB_DURATION_MS.
     * Викликати при нульовому струмі (двигун зупинений, ШІМ вимкнений). */
    float    sum      = 0.0f;
    uint32_t count    = 0U;
    uint32_t start_ms = HWD_Timer_GetMillis();

    while ((HWD_Timer_GetMillis() - start_ms) < CURRENT_CALIB_DURATION_MS) {
        Current_Raw_Data_t raw = {0};
        if (sensor->hw.read_raw(sensor->driver_data, &raw) == SERVO_OK && raw.valid) {
            sum += raw.current_a;
            count++;
        }
    }

    if (count < CURRENT_CALIB_MIN_SAMPLES) return SERVO_ERROR;

    data->zero_offset_a      = sum / (float)count;
    data->filtered_current_a = 0.0f;

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_GetCurrent(const Current_Sensor_Interface_t* sensor,
                                          float* current_a)
{
    if (sensor == NULL || current_a == NULL) return SERVO_ERROR_NULL_PTR;
    *current_a = sensor->data.filtered_current_a;
    return SERVO_OK;
}

Servo_Status_t Current_Sensor_GetPeakCurrent(const Current_Sensor_Interface_t* sensor,
                                               float* peak_a)
{
    if (sensor == NULL || peak_a == NULL) return SERVO_ERROR_NULL_PTR;
    *peak_a = sensor->data.peak_current_a;
    return SERVO_OK;
}

bool Current_Sensor_IsOvercurrent(const Current_Sensor_Interface_t* sensor)
{
    if (sensor == NULL) return false;
    return sensor->data.overcurrent_flag;
}

Servo_Status_t Current_Sensor_ResetPeak(Current_Sensor_Interface_t* sensor)
{
    if (sensor == NULL) return SERVO_ERROR_NULL_PTR;
    sensor->data.peak_current_a   = 0.0f;
    sensor->data.overcurrent_flag = false;
    return SERVO_OK;
}

#endif /* USE_SENSOR_CURRENT */
