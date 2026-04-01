/**
 * @file hwd_pwm.c
 * @brief Реалізація HWD PWM для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * PWM абстракція через libopencm3 timers.
 *
 * Зберігання в HWD_PWM_Config_t:
 *   hw_handle  → (void*)(uintptr_t)TIM3   — базова адреса таймера
 *   hw_channel → (uint32_t)TIM_OC1        — канал (enum tim_oc_id)
 *
 * Передумова: Board_Init() вже налаштував RCC та GPIO alternate functions.
 * HWD_PWM_Init() виконує повну конфігурацію таймера (prescaler, period, OC mode).
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_pwm.h"
#include "./board_config.h"
#include <libopencm3/stm32/timer.h>

/* Private functions ---------------------------------------------------------*/

static inline uint32_t percent_to_duty(float percent, uint32_t resolution)
{
    if (percent < 0.0f)   percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    return (uint32_t)((percent / 100.0f) * (float)resolution);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_PWM_Init(HWD_PWM_Handle_t* handle, const HWD_PWM_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->hw_handle == NULL || config->frequency == 0 || config->resolution == 0) {
        return SERVO_INVALID;
    }

    handle->config       = *config;
    handle->current_duty = 0;
    handle->mode         = HWD_PWM_MODE_FORWARD;
    handle->is_running   = false;

    uint32_t timer  = (uint32_t)(uintptr_t)config->hw_handle;
    enum tim_oc_id oc = (enum tim_oc_id)config->hw_channel;

    /*
     * Розрахунок prescaler:
     *   timer_clock = APB1_TIMER_CLOCK
     *   prescaler   = timer_clock / (frequency * resolution) - 1
     *
     * Для 1 kHz PWM, 1000 кроків, 100 MHz:
     *   prescaler = 100000000 / (1000 * 1000) - 1 = 99
     */
    uint32_t timer_clock = APB1_TIMER_CLOCK;
    uint32_t prescaler   = timer_clock / (config->frequency * config->resolution);

    if (prescaler == 0) {
        return SERVO_INVALID;
    }

    prescaler -= 1;

    /* Конфігурація таймера */
    timer_set_mode(timer,
                   TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE,
                   TIM_CR1_DIR_UP);

    timer_set_prescaler(timer, (uint16_t)prescaler);
    timer_set_period(timer, config->resolution - 1);

    /* Конфігурація PWM виходу */
    timer_set_oc_mode(timer, oc, TIM_OCM_PWM1);
    timer_enable_oc_preload(timer, oc);
    timer_set_oc_value(timer, oc, 0);

    /* Увімкнення auto-reload preload */
    timer_enable_preload(timer);

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_DeInit(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    if (handle->is_running) {
        HWD_PWM_Stop(handle);
    }

    handle->current_duty = 0;
    handle->is_running   = false;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL || handle->config.hw_handle == NULL) {
        return SERVO_INVALID;
    }

    uint32_t timer  = (uint32_t)(uintptr_t)handle->config.hw_handle;
    enum tim_oc_id oc = (enum tim_oc_id)handle->config.hw_channel;

    timer_enable_oc_output(timer, oc);
    timer_enable_counter(timer);

    handle->is_running = true;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Stop(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL || handle->config.hw_handle == NULL) {
        return SERVO_INVALID;
    }

    uint32_t timer  = (uint32_t)(uintptr_t)handle->config.hw_handle;
    enum tim_oc_id oc = (enum tim_oc_id)handle->config.hw_channel;

    /* Спочатку нульовий duty, потім вимкнення виходу */
    timer_set_oc_value(timer, oc, 0);
    timer_disable_oc_output(timer, oc);

    handle->is_running   = false;
    handle->current_duty = 0;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDuty(HWD_PWM_Handle_t* handle, uint32_t duty)
{
    if (handle == NULL || handle->config.hw_handle == NULL) {
        return SERVO_INVALID;
    }

    /* Обмеження в межах resolution */
    if (duty > handle->config.resolution) {
        duty = handle->config.resolution;
    }

    uint32_t timer  = (uint32_t)(uintptr_t)handle->config.hw_handle;
    enum tim_oc_id oc = (enum tim_oc_id)handle->config.hw_channel;

    timer_set_oc_value(timer, oc, duty);

    handle->current_duty = duty;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float percent)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    uint32_t duty = percent_to_duty(percent, handle->config.resolution);

    return HWD_PWM_SetDuty(handle, duty);
}

uint32_t HWD_PWM_GetDuty(const HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return 0;
    }

    return handle->current_duty;
}

Servo_Status_t HWD_PWM_SetFrequency(HWD_PWM_Handle_t* handle, uint32_t frequency)
{
    if (handle == NULL || handle->config.hw_handle == NULL || frequency == 0) {
        return SERVO_INVALID;
    }

    uint32_t timer = (uint32_t)(uintptr_t)handle->config.hw_handle;

    /*
     * Перераховуємо ARR (auto-reload register) зберігаючи поточну resolution.
     * Змінюємо лише period, prescaler лишається.
     *
     * Для зміни лише частоти (не роздільної здатності):
     *   arr = timer_clock / ((prescaler+1) * frequency) - 1
     */
    uint32_t timer_clock = APB1_TIMER_CLOCK;
    uint32_t resolution  = handle->config.resolution;
    uint32_t prescaler   = timer_clock / (frequency * resolution);

    if (prescaler == 0) {
        return SERVO_INVALID;
    }

    prescaler -= 1;

    /* TIM3 — 16-bit, period обмежений 0xFFFF */
    if ((resolution - 1) > 0xFFFFU) {
        return SERVO_ERROR;
    }

    bool was_running = handle->is_running;
    if (was_running) {
        HWD_PWM_Stop(handle);
    }

    timer_set_prescaler(timer, (uint16_t)prescaler);
    timer_set_period(timer, resolution - 1);

    handle->config.frequency = frequency;

    if (was_running) {
        HWD_PWM_Start(handle);
    }

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetMode(HWD_PWM_Handle_t* handle, HWD_PWM_Mode_t mode)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    handle->mode = mode;

    return SERVO_OK;
}

bool HWD_PWM_IsRunning(const HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return false;
    }

    return handle->is_running;
}
