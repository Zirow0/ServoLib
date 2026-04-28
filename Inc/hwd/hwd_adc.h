/**
 * @file hwd_adc.h
 * @brief Апаратна абстракція АЦП з DMA scan режимом
 * @author ServoCore Team
 * @date 2025
 *
 * Послідовність ініціалізації:
 *   1. HWD_ADC_Init(&h1, &cfg1)   — реєстрація каналу 1
 *   2. HWD_ADC_Init(&h2, &cfg2)   — реєстрація каналу 2
 *   ...
 *   N. HWD_ADC_StartScan()        — запуск ADC DMA circular (один раз)
 *
 * Після StartScan() DMA безперервно оновлює буфер усіх каналів.
 * HWD_ADC_ReadVoltage() читає з буфера миттєво, без блокування.
 *
 * Усі канали мають використовувати один і той самий adc_base.
 */

#ifndef SERVOCORE_HWD_ADC_H
#define SERVOCORE_HWD_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація каналу АЦП
 */
typedef struct {
    uint32_t adc_base;      /**< Базова адреса АЦП (платформо-специфічна, напр. ADC1) */
    uint32_t rcc_adc;       /**< RCC для тактування АЦП (напр. RCC_ADC1) */
    uint32_t rcc_gpio;      /**< RCC для GPIO порту */
    uint32_t gpio_port;     /**< Базова адреса GPIO порту (напр. GPIOA) */
    uint32_t gpio_pin;      /**< Маска піна GPIO (напр. GPIO4) */
    uint8_t  channel;       /**< Номер каналу АЦП (0-15) */
    float    vref_v;        /**< Опорна напруга АЦП (В), зазвичай 3.3 */
} HWD_ADC_Config_t;

/**
 * @brief Дескриптор каналу АЦП
 */
typedef struct {
    HWD_ADC_Config_t   config;  /**< Конфігурація */
    volatile uint16_t* raw;     /**< Вказівник на слот у DMA буфері (після Init) */
} HWD_ADC_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Реєстрація каналу АЦП у scan sequence
 *
 * Налаштовує GPIO у режим аналогового входу, додає канал до внутрішнього
 * списку, присвоює handle->raw вказівник на слот у DMA буфері.
 * Викликати для кожного каналу ДО HWD_ADC_StartScan().
 *
 * @param handle Вказівник на дескриптор
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t SERVO_INVALID якщо перевищено HWD_ADC_MAX_CHANNELS
 */
Servo_Status_t HWD_ADC_Init(HWD_ADC_Handle_t* handle, const HWD_ADC_Config_t* config);

/**
 * @brief Запуск ADC DMA circular scan
 *
 * Конфігурує ADC у scan+continuous режимі та запускає DMA circular.
 * Після виклику DMA автоматично оновлює буфер усіх зареєстрованих каналів.
 * Викликати один раз після всіх HWD_ADC_Init().
 *
 * @return Servo_Status_t SERVO_NOT_INIT якщо жодного каналу не зареєстровано
 */
Servo_Status_t HWD_ADC_StartScan(void);

/**
 * @brief Зчитування напруги з DMA буфера (неблокуюче)
 *
 * Повертає останнє значення з DMA буфера у вольтах.
 * Викликати тільки після HWD_ADC_StartScan().
 *
 * @param handle    Вказівник на дескриптор
 * @param voltage_v Результат у вольтах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_ADC_ReadVoltage(HWD_ADC_Handle_t* handle, float* voltage_v);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_ADC_H */
