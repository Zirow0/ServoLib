/**
 * @file hwd_adc.c
 * @brief Реалізація HAL АЦП для STM32F411CEU6 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * Використовує ADC1 у режимі одиночного polling перетворення.
 * Результат повертається у вольтах — розрядність та опорна напруга
 * абстраговані від драйверів верхнього рівня.
 *
 * Підключення (приклад для датчика струму ACS712):
 *   ACS712 OUT → дільник R1/R2 → PA4 (ADC1_IN4)
 *
 * Параметри АЦП:
 *   Тактування:   APB2 (100 МГц) / prescaler 4 = 25 МГц
 *   Розрядність:  12 біт (4096 рівнів)
 *   Час вибірки:  84 цикли → 84/25МГц = 3.36 мкс (достатньо для сигналу після RC-фільтра)
 *   Час конвертації: 12.5 цикли → ~0.5 мкс
 *   Загальний час: ~3.86 мкс << 1 мс (контурний цикл)
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_HWD_ADC

#include "../../Inc/hwd/hwd_adc.h"
#include <libopencm3/stm32/adc.h>

/* Private defines -----------------------------------------------------------*/

/** @brief Розрядність АЦП STM32F411 у режимі 12-bit */
#define HWD_ADC_RESOLUTION_BITS     12U

/** @brief Максимальне значення АЦП (2^12 - 1) */
#define HWD_ADC_MAX_VALUE           4095U

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_ADC_Init(HWD_ADC_Handle_t* handle, const HWD_ADC_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (config->vref_v <= 0.0f) {
        return SERVO_INVALID;
    }

    handle->config         = *config;
    handle->resolution_bits = HWD_ADC_RESOLUTION_BITS;
    handle->is_initialized = false;

    /* Тактування GPIO та АЦП */
    rcc_periph_clock_enable((enum rcc_periph_clken)config->rcc_gpio);
    rcc_periph_clock_enable((enum rcc_periph_clken)config->rcc_adc);

    /* GPIO у режимі аналогового входу (AF не потрібен) */
    gpio_mode_setup((uint32_t)config->gpio_port,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    (uint16_t)config->gpio_pin);

    /* Налаштування АЦП */
    adc_power_off(config->adc_base);

    /* Prescaler: APB2 (100 МГц) / 4 = 25 МГц.
     * Це глобальне налаштування для всіх АЦП — не змінювати між Init викликами. */
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY4);

    adc_disable_scan_mode(config->adc_base);
    adc_set_single_conversion_mode(config->adc_base);
    adc_disable_external_trigger_regular(config->adc_base);
    adc_set_right_aligned(config->adc_base);
    adc_set_resolution(config->adc_base, ADC_CR1_RES_12BIT);

    /* Час вибірки для вказаного каналу */
    adc_set_sample_time(config->adc_base,
                        config->channel,
                        ADC_SMPR_SMP_84CYC);

    adc_power_on(config->adc_base);

    handle->is_initialized = true;
    return SERVO_OK;
}

Servo_Status_t HWD_ADC_DeInit(HWD_ADC_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!handle->is_initialized) {
        return SERVO_NOT_INIT;
    }

    adc_power_off(handle->config.adc_base);
    handle->is_initialized = false;

    return SERVO_OK;
}

Servo_Status_t HWD_ADC_ReadVoltage(HWD_ADC_Handle_t* handle, float* voltage_v)
{
    if (handle == NULL || voltage_v == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!handle->is_initialized) {
        return SERVO_NOT_INIT;
    }

    /* Встановлення каналу та запуск одиночного перетворення */
    uint8_t channel_seq[1] = { (uint8_t)handle->config.channel };
    adc_set_regular_sequence(handle->config.adc_base, 1, channel_seq);
    adc_start_conversion_regular(handle->config.adc_base);

    /* Очікування завершення перетворення (EOC) */
    while (!adc_eoc(handle->config.adc_base)) {
        /* Polling — прийнятно для коротких (<4 мкс) перетворень.
         * При необхідності додати таймаут через HWD_Timer_GetMicros(). */
    }

    /* Зчитування результату та конвертація у вольти */
    uint16_t raw = adc_read_regular(handle->config.adc_base);
    *voltage_v = (float)raw * handle->config.vref_v / (float)HWD_ADC_MAX_VALUE;

    return SERVO_OK;
}

#endif /* USE_HWD_ADC */
