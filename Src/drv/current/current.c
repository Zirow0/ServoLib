/**
 * @file current.c
 * @brief Реалізація базової логіки датчика струму
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

#define CURRENT_CALIB_N_SAMPLES     20U    /**< Кількість вимірів для калібрування */
#define CURRENT_CALIB_SAMPLE_MS     5U     /**< Пауза між вимірами (мс) */
#define CURRENT_UKF_DT              0.001f /**< Крок часу UKF (1 кГц контурний цикл) */

/* Private functions ---------------------------------------------------------*/

static void Current_UKF_StateFunc(const float* x, float dt, float* x_pred, void* user_data)
{
    (void)dt;
    (void)user_data;
    x_pred[0] = x[0];  /* Модель випадкового блукання: струм зберігається */
}

static void Current_UKF_MeasurementFunc(const float* x, float* z_pred, void* user_data)
{
    (void)user_data;
    z_pred[0] = x[0];  /* Спостерігаємо струм напряму */
}

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

    memset(&sensor->data, 0, sizeof(Current_Sensor_Data_t));

    sensor->data.overcurrent_threshold_a = params->overcurrent_threshold_a;

    if (sensor->hw.init != NULL) {
        Servo_Status_t status = sensor->hw.init(sensor->driver_data, params);
        if (status != SERVO_OK) {
            return status;
        }
    }

    /* Ініціалізація UKF: стан [струм_А], вимірювання [струм_А] */
    float x0[1] = {0.0f};
    float P0[1] = {1.0f};
    float Q[1]  = {params->process_noise_q};
    float R[1]  = {params->measurement_noise_r};

    ukf_init(&sensor->data.ukf, 1, 1,
             Current_UKF_StateFunc, Current_UKF_MeasurementFunc, NULL,
             sensor->data.ukf_buf);
    ukf_set_params(&sensor->data.ukf, 0.5f, 2.0f, 0.0f);
    ukf_set_state(&sensor->data.ukf, x0, P0);
    ukf_set_noise(&sensor->data.ukf, Q, R);

    return SERVO_OK;
}

Servo_Status_t Current_Sensor_Update(Current_Sensor_Interface_t* sensor)
{
    if (!Current_Sensor_IsValid(sensor)) return SERVO_INVALID;

    Current_Sensor_Data_t* data = &sensor->data;

    Current_Raw_Data_t raw;
    memset(&raw, 0, sizeof(Current_Raw_Data_t));

    Servo_Status_t status = sensor->hw.read_raw(sensor->driver_data, &raw);
    if (status != SERVO_OK) return status;

    if (!raw.valid) return SERVO_ERROR;

    float current = raw.current_a - data->zero_offset_a;

    /* UKF фільтрація */
    ukf_predict(&data->ukf, CURRENT_UKF_DT);
    ukf_update(&data->ukf, &current);

    float ukf_state[1];
    ukf_get_state(&data->ukf, ukf_state, NULL);
    data->filtered_current_a = ukf_state[0];

    float abs_current = (current < 0.0f) ? -current : current;
    if (abs_current > data->peak_current_a) {
        data->peak_current_a = abs_current;
    }

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

    /* Програмне калібрування: CURRENT_CALIB_N_SAMPLES вимірів з паузою між ними.
     * Викликати при нульовому струмі (двигун зупинений, ШІМ вимкнений). */
    float    sum   = 0.0f;
    uint32_t count = 0U;

    for (uint32_t i = 0U; i < CURRENT_CALIB_N_SAMPLES; i++) {
        Current_Raw_Data_t raw = {0};
        if (sensor->hw.read_raw(sensor->driver_data, &raw) == SERVO_OK && raw.valid) {
            sum += raw.current_a;
            count++;
        }
        HWD_Timer_DelayMs(CURRENT_CALIB_SAMPLE_MS);
    }

    if (count == 0U) return SERVO_ERROR;

    data->zero_offset_a      = sum / (float)count;
    data->filtered_current_a = 0.0f;

    /* Скидання стану UKF після визначення нуля */
    float x0[1] = {0.0f};
    float P0[1] = {1.0f};
    ukf_set_state(&data->ukf, x0, P0);

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
