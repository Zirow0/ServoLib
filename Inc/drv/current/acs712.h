/**
 * @file acs712.h
 * @brief Драйвер датчика струму ACS712T (ефект Хола, аналоговий вихід)
 * @author ServoCore Team
 * @date 2025
 *
 * Hardware Callbacks Pattern: тільки апаратні операції (читання АЦП, конвертація
 * напруги у струм). Вся обробка (фільтрація, калібрування, захист) — у current.c.
 *
 * Підтримувані варіанти:
 *   ACS712_05B  ±5А   185 мВ/А
 *   ACS712_20A  ±20А  100 мВ/А
 *   ACS712_30A  ±30А  66.6 мВ/А
 *
 * Схема підключення:
 *   ACS712 VCC   → 5В
 *   ACS712 OUT   → R1 → ADC_IN (0..3.3В)
 *                  R2 → GND
 *   Дільник: R1=3.4кОм, R2=6.8кОм → k=0.667 (5В → 3.3В)
 *   RC-фільтр: після дільника 1кОм + 100нФ → F_зрізу ≈ 1.6кГц
 *
 * Формула конвертації (у read_raw):
 *   I_raw = Vadc × current_coefficient
 *   де current_coefficient = 1 / (sensitivity_V_per_A × divider_ratio)
 *   Зміщення нуля компенсується калібруванням у current.c (zero_offset_a).
 */

#ifndef SERVOCORE_DRV_ACS712_H
#define SERVOCORE_DRV_ACS712_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "current.h"
#include "../../hwd/hwd_adc.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Варіант датчика ACS712T
 *
 * Визначає чутливість та максимальний струм.
 */
typedef enum {
    ACS712_05B = 0,  /**< ±5А,  185 мВ/А */
    ACS712_20A = 1,  /**< ±20А, 100 мВ/А */
    ACS712_30A = 2,  /**< ±30А, 66.6 мВ/А */
} ACS712_Variant_t;

/**
 * @brief Конфігурація драйвера ACS712T
 */
typedef struct {
    ACS712_Variant_t  variant;               /**< Варіант датчика */
    HWD_ADC_Handle_t* adc;                   /**< Ініціалізований дескриптор каналу АЦП */
    float             divider_ratio;         /**< Коефіцієнт дільника (< 1.0), напр. 0.66 для 5В→3.3В */
    float             overcurrent_threshold_a; /**< Поріг перевантаження (А), 0.0 = вимкнено */
    float             ema_alpha;             /**< Коефіцієнт EMA фільтра [0.0 - 1.0] */
} ACS712_Config_t;

/**
 * @brief Структура драйвера ACS712T
 *
 * ВАЖЛИВО: Current_Sensor_Interface_t — ПЕРШЕ поле.
 * Дозволяє передавати &driver.interface у Servo_InitFull та Current_Sensor_*().
 */
typedef struct {
    Current_Sensor_Interface_t interface;  /**< Універсальний інтерфейс (ПЕРШИМ!) */
    float             current_coefficient; /**< 1/(sensitivity × divider_ratio), А/В */
    HWD_ADC_Handle_t* adc;                /**< Дескриптор каналу АЦП */
} ACS712_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера ACS712T
 *
 * Обчислює current_coefficient та max_current_a з варіанту.
 * Налаштовує hardware callbacks. Викликає Current_Sensor_Init().
 *
 * Після створення:
 *   - Current_Sensor_Calibrate(&driver.interface) при нульовому струмі
 *   - Current_Sensor_Update(&driver.interface) у контурному циклі
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Конфігурація датчика та базового шару
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t ACS712_Create(ACS712_Driver_t* driver, const ACS712_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_ACS712_H */
