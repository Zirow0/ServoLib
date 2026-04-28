/**
 * @file hwd_adc.c
 * @brief Реалізація HAL АЦП для STM32F411CEU6 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * ADC1 у scan+continuous режимі з DMA2 Stream0 Channel0 (circular).
 * DMA безперервно оновлює s_dma_buf[] — кожен канал має свій слот.
 * HWD_ADC_ReadVoltage() читає з буфера миттєво, без blocking.
 *
 * Параметри АЦП:
 *   Тактування:  APB2 (100 МГц) / prescaler 4 = 25 МГц
 *   Розрядність: 12 біт (4096 рівнів)
 *   Час вибірки: 84 цикли → ~3.36 мкс/канал (достатньо після RC-фільтра)
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_HWD_ADC

#include "../../Inc/hwd/hwd_adc.h"
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* Private defines -----------------------------------------------------------*/

#define HWD_ADC_MAX_CHANNELS  8U    /**< Максимум каналів у scan sequence */
#define HWD_ADC_MAX_VALUE     4095U /**< 2^12 - 1 */

/* Private data --------------------------------------------------------------*/

/** @brief DMA буфер — кожен слот оновлюється DMA автоматично */
static volatile uint16_t s_dma_buf[HWD_ADC_MAX_CHANNELS];

/** @brief Список зареєстрованих каналів (для adc_set_regular_sequence) */
static uint8_t s_channels[HWD_ADC_MAX_CHANNELS];

/** @brief Кількість зареєстрованих каналів */
static uint8_t s_channel_count = 0U;

/** @brief Базова адреса АЦП (спільна для всіх каналів) */
static uint32_t s_adc_base = 0U;

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_ADC_Init(HWD_ADC_Handle_t* handle, const HWD_ADC_Config_t* config)
{
    if (handle == NULL || config == NULL) return SERVO_ERROR_NULL_PTR;
    if (config->vref_v <= 0.0f)          return SERVO_INVALID;
    if (s_channel_count >= HWD_ADC_MAX_CHANNELS) return SERVO_INVALID;

    handle->config = *config;
    handle->raw    = &s_dma_buf[s_channel_count];

    /* GPIO у режимі аналогового входу */
    rcc_periph_clock_enable((enum rcc_periph_clken)config->rcc_gpio);
    gpio_mode_setup(config->gpio_port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                    (uint16_t)config->gpio_pin);

    /* Тактування АЦП (безпечно викликати кілька разів) */
    rcc_periph_clock_enable((enum rcc_periph_clken)config->rcc_adc);

    s_channels[s_channel_count] = config->channel;
    s_adc_base                  = config->adc_base;
    s_channel_count++;

    return SERVO_OK;
}

Servo_Status_t HWD_ADC_StartScan(void)
{
    if (s_channel_count == 0U) return SERVO_NOT_INIT;

    /* ── DMA2 Stream0 Channel0 (ADC1) ──────────────────────────────────── */
    rcc_periph_clock_enable(RCC_DMA2);

    dma_stream_reset(DMA2, DMA_STREAM0);
    dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
    dma_set_transfer_mode(DMA2, DMA_STREAM0, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_priority(DMA2, DMA_STREAM0, DMA_SxCR_PL_HIGH);
    dma_set_peripheral_size(DMA2, DMA_STREAM0, DMA_SxCR_PSIZE_16BIT);
    dma_set_memory_size(DMA2, DMA_STREAM0, DMA_SxCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA2, DMA_STREAM0);
    dma_enable_circular_mode(DMA2, DMA_STREAM0);
    dma_set_peripheral_address(DMA2, DMA_STREAM0, (uint32_t)&ADC_DR(s_adc_base));
    dma_set_memory_address(DMA2, DMA_STREAM0, (uint32_t)s_dma_buf);
    dma_set_number_of_data(DMA2, DMA_STREAM0, s_channel_count);
    dma_enable_stream(DMA2, DMA_STREAM0);

    /* ── ADC scan + continuous + DMA ───────────────────────────────────── */
    adc_power_off(s_adc_base);
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY4);   /* 25 МГц */
    adc_enable_scan_mode(s_adc_base);
    adc_set_continuous_conversion_mode(s_adc_base);
    adc_disable_external_trigger_regular(s_adc_base);
    adc_set_right_aligned(s_adc_base);
    adc_set_resolution(s_adc_base, ADC_CR1_RES_12BIT);

    for (uint8_t i = 0; i < s_channel_count; i++) {
        adc_set_sample_time(s_adc_base, s_channels[i], ADC_SMPR_SMP_84CYC);
    }

    adc_set_regular_sequence(s_adc_base, s_channel_count, s_channels);
    adc_enable_dma(s_adc_base);
    adc_set_dma_continue(s_adc_base);   /* DDS: продовжувати DMA запити */

    adc_power_on(s_adc_base);
    adc_start_conversion_regular(s_adc_base);

    return SERVO_OK;
}

Servo_Status_t HWD_ADC_ReadVoltage(HWD_ADC_Handle_t* handle, float* voltage_v)
{
    if (handle == NULL || voltage_v == NULL) return SERVO_ERROR_NULL_PTR;
    if (handle->raw == NULL)                 return SERVO_NOT_INIT;

    *voltage_v = (float)*handle->raw * handle->config.vref_v / (float)HWD_ADC_MAX_VALUE;
    return SERVO_OK;
}

#endif /* USE_HWD_ADC */
