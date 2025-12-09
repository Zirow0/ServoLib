/**
 * @file hwd_pwm.c
 * @brief Реалізація HWD PWM для STM32F411
 * @author ServoCore Team
 * @date 2025
 *
 * Платформо-специфічна реалізація PWM абстракції для STM32F4xx.
 */

/* Includes ------------------------------------------------------------------*/
#include "hwd/hwd_pwm.h"
#include "./board_config.h"

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Перетворення відсотків у duty cycle
 */
static inline uint32_t PercentToDuty(float percent, uint32_t resolution)
{
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    return (uint32_t)((percent / 100.0f) * (float)resolution);
}

/**
 * @brief Перетворення duty cycle у відсотки
 */
static inline float DutyToPercent(uint32_t duty, uint32_t resolution)
{
    if (resolution == 0) return 0.0f;
    return ((float)duty / (float)resolution) * 100.0f;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_PWM_Init(HWD_PWM_Handle_t* handle, const HWD_PWM_Config_t* config)
{
    if (handle == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    if (config->hw_handle == NULL) {
        return SERVO_INVALID;
    }

    // Копіювання конфігурації
    handle->config = *config;
    handle->current_duty = 0;
    handle->mode = HWD_PWM_MODE_FORWARD;
    handle->is_running = false;

    // STM32 HAL вже ініціалізовано через CubeMX
    // Встановлюємо початковий duty cycle на 0
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)config->hw_handle;
    __HAL_TIM_SET_COMPARE(htim, config->hw_channel, 0);

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_DeInit(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    // Зупинка PWM
    if (handle->is_running) {
        HWD_PWM_Stop(handle);
    }

    // Очищення структури
    handle->current_duty = 0;
    handle->is_running = false;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)handle->config.hw_handle;

    if (htim == NULL) {
        return SERVO_INVALID;
    }

    // Запуск PWM через STM32 HAL
    HAL_StatusTypeDef hal_status = HAL_TIM_PWM_Start(htim, handle->config.hw_channel);

    if (hal_status != HAL_OK) {
        return SERVO_ERROR;
    }

    handle->is_running = true;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_Stop(HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)handle->config.hw_handle;

    if (htim == NULL) {
        return SERVO_INVALID;
    }

    // Встановлення duty cycle на 0
    __HAL_TIM_SET_COMPARE(htim, handle->config.hw_channel, 0);

    // Зупинка PWM через STM32 HAL
    HAL_StatusTypeDef hal_status = HAL_TIM_PWM_Stop(htim, handle->config.hw_channel);

    if (hal_status != HAL_OK) {
        return SERVO_ERROR;
    }

    handle->is_running = false;
    handle->current_duty = 0;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDuty(HWD_PWM_Handle_t* handle, uint32_t duty)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)handle->config.hw_handle;

    if (htim == NULL) {
        return SERVO_INVALID;
    }

    // Обмеження duty в межах resolution
    if (duty > handle->config.resolution) {
        duty = handle->config.resolution;
    }

    // Встановлення compare value для PWM
    __HAL_TIM_SET_COMPARE(htim, handle->config.hw_channel, duty);

    handle->current_duty = duty;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float percent)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    uint32_t duty = PercentToDuty(percent, handle->config.resolution);

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
    if (handle == NULL || frequency == 0) {
        return SERVO_INVALID;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)handle->config.hw_handle;

    if (htim == NULL) {
        return SERVO_INVALID;
    }

    // Розрахунок нового ARR (Auto-Reload Register)
    // Timer Clock = APB1_TIMER_CLOCK / (Prescaler + 1)
    // PWM Frequency = Timer Clock / (ARR + 1)
    // ARR = (Timer Clock / PWM Frequency) - 1

    uint32_t timer_clock = APB1_TIMER_CLOCK / (htim->Init.Prescaler + 1);
    uint32_t arr = (timer_clock / frequency) - 1;

    // Перевірка меж
    if (arr > 0xFFFF) {  // 16-bit таймер
        return SERVO_ERROR;
    }

    // Зупинка таймера
    bool was_running = handle->is_running;
    if (was_running) {
        HWD_PWM_Stop(handle);
    }

    // Встановлення нового періоду
    __HAL_TIM_SET_AUTORELOAD(htim, arr);

    // Оновлення resolution
    handle->config.resolution = arr;

    // Запуск якщо був активний
    if (was_running) {
        HWD_PWM_Start(handle);
    }

    handle->config.frequency = frequency;

    return SERVO_OK;
}

Servo_Status_t HWD_PWM_SetMode(HWD_PWM_Handle_t* handle, HWD_PWM_Mode_t mode)
{
    if (handle == NULL) {
        return SERVO_INVALID;
    }

    handle->mode = mode;

    // Режими можуть використовуватися драйверами вищого рівня
    // Тут просто зберігаємо стан

    return SERVO_OK;
}

bool HWD_PWM_IsRunning(const HWD_PWM_Handle_t* handle)
{
    if (handle == NULL) {
        return false;
    }

    return handle->is_running;
}
