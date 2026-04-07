/**
 * @file hwd_timer.h
 * @brief Апаратна абстракція таймерів та затримок
 * @author ServoCore Team
 * @date 2025
 *
 * Незалежний від платформи інтерфейс для роботи з таймерами
 */

#ifndef SERVOCORE_HWD_TIMER_H
#define SERVOCORE_HWD_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Отримання часу в мілісекундах
 *
 * Повертає час з моменту старту системи в мілісекундах.
 * Використовується для вимірювання інтервалів та таймаутів.
 *
 * @return uint32_t Час в мілісекундах
 */
uint32_t HWD_Timer_GetMillis(void);

/**
 * @brief Отримання часу в мікросекундах
 *
 * Повертає час з моменту старту системи в мікросекундах.
 * Використовується для високоточних вимірювань.
 *
 * @return uint32_t Час в мікросекундах
 */
uint32_t HWD_Timer_GetMicros(void);

/**
 * @brief Затримка в мілісекундах
 *
 * Блокуюча затримка на вказану кількість мілісекунд.
 * УВАГА: Блокує виконання програми!
 *
 * @param ms Час затримки в мілісекундах
 */
void HWD_Timer_DelayMs(uint32_t ms);

/**
 * @brief Затримка в мікросекундах
 *
 * Блокуюча затримка на вказану кількість мікросекунд.
 * УВАГА: Блокує виконання програми!
 *
 * @param us Час затримки в мікросекундах
 */
void HWD_Timer_DelayUs(uint32_t us);

/**
 * @brief Перевірка чи минув інтервал часу
 *
 * Допоміжна функція для неблокуючих затримок.
 * Правильно обробляє переповнення лічильника.
 *
 * @param start Початковий час (GetMillis)
 * @param interval Інтервал в мілісекундах
 * @return bool true якщо інтервал минув
 */
bool HWD_Timer_IsElapsed(uint32_t start, uint32_t interval);

/**
 * @brief Обчислення різниці часу
 *
 * Обчислює різницю між двома моментами часу.
 * Правильно обробляє переповнення лічильника.
 *
 * @param start Початковий час
 * @param end Кінцевий час
 * @return uint32_t Різниця в мілісекундах
 */
uint32_t HWD_Timer_GetElapsed(uint32_t start, uint32_t end);

/* ── Encoder timer (квадратурний режим) ─────────────────────────────────── */

/**
 * @brief Конфігурація таймера в режимі квадратурного енкодера
 *
 * Приклад для TIM2 на STM32F411 (PA0=CH1, PA1=CH2, AF1):
 * @code
 * HWD_Encoder_Config_t cfg = {
 *     .timer_base  = TIM2,   .rcc_timer  = RCC_TIM2,
 *     .gpio_port_a = GPIOA,  .gpio_pin_a = GPIO0,  .rcc_gpio_a = RCC_GPIOA,
 *     .gpio_port_b = GPIOA,  .gpio_pin_b = GPIO1,  .rcc_gpio_b = RCC_GPIOA,
 *     .gpio_af     = GPIO_AF1,
 * };
 * @endcode
 */
typedef struct {
    uint32_t timer_base;    /**< Базова адреса таймера */
    uint32_t rcc_timer;     /**< RCC таймера */
    uint32_t gpio_port_a;   /**< GPIO порт каналу A */
    uint16_t gpio_pin_a;    /**< GPIO пін каналу A */
    uint32_t rcc_gpio_a;    /**< RCC GPIO каналу A */
    uint32_t gpio_port_b;   /**< GPIO порт каналу B */
    uint16_t gpio_pin_b;    /**< GPIO пін каналу B */
    uint32_t rcc_gpio_b;    /**< RCC GPIO каналу B */
    uint8_t  gpio_af;       /**< Alternate Function */
    bool     invert_a;      /**< Інвертувати полярність каналу A */
    bool     invert_b;      /**< Інвертувати полярність каналу B */
} HWD_Encoder_Config_t;

/** @brief Дескриптор таймера-енкодера (заповнюється при Init) */
typedef struct {
    uint32_t timer_base;
} HWD_Encoder_Handle_t;

/**
 * @brief Ініціалізація таймера в режимі квадратурного енкодера (x4)
 *
 * Включає тактування, конфігурує GPIO як AF-входи з підтяжкою вверх,
 * встановлює encoder mode 3 (лічить обидва фронти обох каналів), скидає лічильник.
 */
Servo_Status_t HWD_Timer_EncoderInit(HWD_Encoder_Handle_t* handle,
                                      const HWD_Encoder_Config_t* config);

/** @brief Зупинка таймера-енкодера */
Servo_Status_t HWD_Timer_EncoderDeInit(HWD_Encoder_Handle_t* handle);

/**
 * @brief Зчитування лічильника (знаковий int32)
 *
 * Вперед — збільшується, назад — зменшується.
 * TIM2 (32-bit): діапазон ±2 млрд відліків (≈ ±1M обертів при CPR=2400).
 */
int32_t HWD_Timer_EncoderRead(const HWD_Encoder_Handle_t* handle);

/** @brief Скидання лічильника до 0 (встановлення нульової позиції) */
void HWD_Timer_EncoderReset(HWD_Encoder_Handle_t* handle);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_TIMER_H */
