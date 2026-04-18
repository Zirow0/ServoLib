/**
 * @file current.c
 * @brief Реалізація базової логіки датчика струму
 * @author ServoCore Team
 * @date 2025
 *
 * Спільна логіка для всіх типів датчиків струму:
 *   - EMA фільтрація
 *   - Корекція нульового зміщення (zero_offset_a)
 *   - Відстеження пікового струму
 *   - Виявлення перевантаження
 *   - Визначення напрямку
 *   - Програмне калібрування через усереднення read_raw
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_CURRENT

/* Auto-enable якщо підключено конкретний драйвер */
#include "drv/current/current.h"
#include "hwd/hwd_timer.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/** @brief Тривалість програмного калібрування (мс) */
#define CURRENT_CALIB_DURATION_MS   50U

/** @brief Мінімальна кількість зразків для валідного калібрування */
#define CURRENT_CALIB_MIN_SAMPLES   10U

/** @brief Мертва зона для визначення напрямку (А) */
#define CURRENT_DIRECTION_DEADBAND  0.05f

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

    sensor->data.is_initialized = true;
    return SERVO_OK;
}

Servo_Status_t Current_Sensor_DeInit(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Current_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    if (sensor->hw.deinit != NULL) {
        Servo_Status_t status = sensor->hw.deinit(sensor->driver_data);
        if (status != SERVO_OK) {
            return status;
        }
    }

    memset(data, 0, sizeof(Current_Sensor_Data_t));

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_Update(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Current_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    /* 1. Зчитати миттєвий струм з драйвера */
    Current_Raw_Data_t raw;
    memset(&raw, 0, sizeof(Current_Raw_Data_t));

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
        if (!data->overcurrent_flag) {
            data->stats.overcurrent_count++;
        }
        data->overcurrent_flag = true;
    }

    /* 6. Визначення напрямку */
    if (current > CURRENT_DIRECTION_DEADBAND) {
        data->direction = CURRENT_DIR_POSITIVE;
    } else if (current < -CURRENT_DIRECTION_DEADBAND) {
        data->direction = CURRENT_DIR_NEGATIVE;
    } else {
        data->direction = CURRENT_DIR_ZERO;
    }

    /* 7. Оновлення статистики */
    data->stats.update_count++;
    data->stats.last_current_a = current;
    data->stats.peak_current_a = data->peak_current_a;

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_Calibrate(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) {
        return SERVO_INVALID;
    }

    Current_Sensor_Data_t* data = &sensor->data;

    if (!data->is_initialized) {
        return SERVO_NOT_INIT;
    }

    /* Якщо є апаратне калібрування — делегуємо */
    if (sensor->hw.calibrate != NULL) {
        Servo_Status_t status = sensor->hw.calibrate(sensor->driver_data);
        if (status != SERVO_OK) {
            data->last_error = ERR_SENSOR_CALIB_FAILED;
            return status;
        }
        data->is_calibrated       = true;
        data->stats.is_calibrated = true;
        return SERVO_OK;
    }

    /* Програмне калібрування: усереднення read_raw за CURRENT_CALIB_DURATION_MS.
     * Викликати при нульовому струмі (двигун зупинений, ШІМ вимкнений).
     * Кожен виклик read_raw — блокуючий polling АЦП, затримка між
     * викликами не потрібна (АЦП повертає новий результат щоразу). */
    float    sum   = 0.0f;
    uint32_t count = 0U;
    uint32_t start_ms = HWD_Timer_GetMillis();

    while ((HWD_Timer_GetMillis() - start_ms) < CURRENT_CALIB_DURATION_MS) {
        Current_Raw_Data_t raw;
        memset(&raw, 0, sizeof(Current_Raw_Data_t));

        if (sensor->hw.read_raw(sensor->driver_data, &raw) == SERVO_OK && raw.valid) {
            sum += raw.current_a;
            count++;
        }
    }

    if (count < CURRENT_CALIB_MIN_SAMPLES) {
        data->last_error = ERR_SENSOR_CALIB_FAILED;
        return SERVO_ERROR;
    }

    data->zero_offset_a       = sum / (float)count;
    data->filtered_current_a  = 0.0f;
    data->is_calibrated       = true;
    data->stats.is_calibrated = true;

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_GetCurrent(const Current_Sensor_Interface_t* sensor,
                                          float* current_a)
{
    if (sensor == NULL || current_a == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!sensor->data.is_initialized) {
        return SERVO_NOT_INIT;
    }

    *current_a = sensor->data.filtered_current_a;
    return SERVO_OK;
}

Servo_Status_t Current_Sensor_GetPeakCurrent(const Current_Sensor_Interface_t* sensor,
                                               float* peak_a)
{
    if (sensor == NULL || peak_a == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!sensor->data.is_initialized) {
        return SERVO_NOT_INIT;
    }

    *peak_a = sensor->data.peak_current_a;
    return SERVO_OK;
}

bool Current_Sensor_IsOvercurrent(const Current_Sensor_Interface_t* sensor)
{
    if (sensor == NULL) {
        return false;
    }

    return sensor->data.overcurrent_flag;
}

Servo_Status_t Current_Sensor_ResetPeak(Current_Sensor_Interface_t* sensor)
{
    if (sensor == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!sensor->data.is_initialized) {
        return SERVO_NOT_INIT;
    }

    sensor->data.peak_current_a         = 0.0f;
    sensor->data.overcurrent_flag       = false;
    sensor->data.stats.peak_current_a   = 0.0f;

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_GetStats(const Current_Sensor_Interface_t* sensor,
                                        Current_Stats_t* stats)
{
    if (sensor == NULL || stats == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!sensor->data.is_initialized) {
        return SERVO_NOT_INIT;
    }

    *stats = sensor->data.stats;
    return SERVO_OK;
}

#endif /* USE_SENSOR_CURRENT */
