/**
 * @file hwd_adc.h
 * @brief Апаратна абстракція аналого-цифрового перетворювача (АЦП)
 * @author ServoCore Team
 * @date 2025
 *
 * Платформонезалежний інтерфейс для роботи з АЦП.
 * Конкретна реалізація — Board/STM32F411_OCM3/hwd_adc.c (libopencm3).
 *
 * HWD_ADC_ReadVoltage() повертає напругу у вольтах — драйвери верхнього рівня
 * не знають про розрядність АЦП та опорну напругу конкретної платформи.
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
    HWD_ADC_Config_t config;    /**< Конфігурація */
    uint8_t  resolution_bits;   /**< Розрядність АЦП, заповнюється при Init */
    bool     is_initialized;    /**< Прапорець ініціалізації */
} HWD_ADC_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація каналу АЦП
 *
 * Налаштовує GPIO у режим аналогового входу, конфігурує АЦП
 * (розрядність, час вибірки, одиночне перетворення).
 *
 * @param handle Вказівник на дескриптор
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_ADC_Init(HWD_ADC_Handle_t* handle, const HWD_ADC_Config_t* config);

/**
 * @brief Деініціалізація каналу АЦП
 *
 * @param handle Вказівник на дескриптор
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_ADC_DeInit(HWD_ADC_Handle_t* handle);

/**
 * @brief Зчитування напруги (блокуючий режим)
 *
 * Запускає одиночне перетворення, очікує завершення, повертає результат
 * у вольтах. Розрядність та опорна напруга абстраговані всередині.
 *
 * Використовувати:
 *  - під час калібрування (ШІМ вимкнений)
 *  - при опитуванні без прив'язки до ШІМ
 *
 * @param handle   Вказівник на дескриптор
 * @param voltage_v Результат у вольтах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_ADC_ReadVoltage(HWD_ADC_Handle_t* handle, float* voltage_v);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_ADC_H */
