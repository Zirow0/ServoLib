/**
 * @file acs712.c
 * @brief Реалізація драйвера датчика струму ACS712T
 * @author ServoCore Team
 * @date 2025
 *
 * Hardware Callbacks Pattern: тільки апаратні операції.
 * Читає АЦП → конвертує напругу у Ампери → повертає миттєве значення.
 * Фільтрація, калібрування, захист — відповідальність current.c.
 *
 * Конвертація:
 *   I_raw = Vadc × current_coefficient
 *   current_coefficient = 1 / (sensitivity_V_per_A × divider_ratio)
 *
 *   Зміщення нуля (Vzero = Vcc/2 × divider_ratio) не вираховується тут —
 *   воно поглинається zero_offset_a під час калібрування у current.c.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_ACS712

/* Auto-enable базового шару */
#ifndef USE_SENSOR_CURRENT
    #define USE_SENSOR_CURRENT
#endif

#include "drv/current/acs712.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/** @brief Кількість варіантів ACS712 */
#define ACS712_VARIANT_COUNT    3U

/* Private data --------------------------------------------------------------*/

/**
 * @brief Таблиця параметрів варіантів ACS712T
 *
 * sensitivity_v_per_a: чутливість виходу датчика (В/А)
 * max_current_a:       максимальний номінальний струм (А)
 */
static const struct {
    float sensitivity_v_per_a;
    float max_current_a;
} acs712_variants[ACS712_VARIANT_COUNT] = {
    [ACS712_05B] = { .sensitivity_v_per_a = 0.185f, .max_current_a =  5.0f },
    [ACS712_20A] = { .sensitivity_v_per_a = 0.100f, .max_current_a = 20.0f },
    [ACS712_30A] = { .sensitivity_v_per_a = 0.0666f,.max_current_a = 30.0f },
};

/* Private hardware callbacks ------------------------------------------------*/

/**
 * @brief Hardware callback: ініціалізація АЦП каналу
 */
static Servo_Status_t ACS712_HW_Init(void* driver_data, const Current_Params_t* params)
{
    ACS712_Driver_t* driver = (ACS712_Driver_t*)driver_data;

    if (driver == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    (void)params;  /* АЦП налаштований через HWD_ADC_Handle_t */

    /* АЦП зареєстрований через HWD_ADC_Init() перед Create — raw != NULL. */
    if (driver->adc == NULL || driver->adc->raw == NULL) {
        return SERVO_NOT_INIT;
    }

    return SERVO_OK;
}

/**
 * @brief Hardware callback: зчитування миттєвого струму
 *
 * Виконує блокуючий polling АЦП. Підходить для:
 *   - калібрування (ШІМ вимкнений)
 *   - роботи з RC-фільтром на виході датчика (апаратна фільтрація)
 *
 * I_raw = Vadc × current_coefficient
 * Зміщення нуля НЕ вираховується — це робить current.c через zero_offset_a.
 */
static Servo_Status_t ACS712_HW_ReadRaw(void* driver_data, Current_Raw_Data_t* raw)
{
    ACS712_Driver_t* driver = (ACS712_Driver_t*)driver_data;

    if (driver == NULL || raw == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    raw->valid = false;

    /* Читання напруги з АЦП (блокуючий polling) */
    float vadc_v = 0.0f;
    Servo_Status_t status = HWD_ADC_ReadVoltage(driver->adc, &vadc_v);
    if (status != SERVO_OK) {
        return status;
    }

    /* Конвертація: I_raw = Vadc × current_coefficient */
    raw->current_a = vadc_v * driver->current_coefficient;
    raw->valid     = true;

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t ACS712_Create(ACS712_Driver_t* driver, const ACS712_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (config->adc == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if ((uint32_t)config->variant >= ACS712_VARIANT_COUNT) {
        return SERVO_INVALID;
    }

    if (config->divider_ratio <= 0.0f || config->divider_ratio > 1.0f) {
        return SERVO_INVALID;
    }

    /* Очищення структури */
    memset(driver, 0, sizeof(ACS712_Driver_t));

    /* Обчислення коефіцієнту конвертації з параметрів варіанту:
     * current_coefficient = 1 / (sensitivity × divider_ratio)
     * Одиниця: А/В — множиться на напругу АЦП, результат у Амперах. */
    float sensitivity = acs712_variants[config->variant].sensitivity_v_per_a;
    driver->current_coefficient   = 1.0f / (sensitivity * config->divider_ratio);
    driver->adc                   = config->adc;

    /* Налаштування hardware callbacks */
    driver->interface.hw.init     = ACS712_HW_Init;
    driver->interface.hw.read_raw = ACS712_HW_ReadRaw;

    /* Вказівник для callbacks */
    driver->interface.driver_data = driver;

    /* Ініціалізація базового шару */
    Current_Params_t base_params = {
        .overcurrent_threshold_a = config->overcurrent_threshold_a,
        .ema_alpha               = config->ema_alpha,
    };

    return Current_Sensor_Init(&driver->interface, &base_params);
}

#endif /* USE_SENSOR_ACS712 */
