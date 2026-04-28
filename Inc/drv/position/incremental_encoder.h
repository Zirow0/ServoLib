/**
 * @file incremental_encoder.h
 * @brief Драйвер інкрементального квадратурного енкодера (EXTI + Input Capture)
 *
 * Позиція: EXTI на обох фронтах каналів A і B → X4 state machine → volatile int32_t count
 * Швидкість: TIM Input Capture на каналі A (наростаючий фронт) → volatile uint32_t period_us
 *
 * Board ISR викликає Incremental_Encoder_EXTI_Handler та Incremental_Encoder_IC_Handler.
 * count — 32-bit знаковий, необмежений діапазон (підходить для 6 датчиків без 32-bit таймерів).
 */

#ifndef SERVOCORE_DRV_INCREMENTAL_ENCODER_H
#define SERVOCORE_DRV_INCREMENTAL_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "position.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура драйвера інкрементального енкодера
 *
 * Перше поле — Position_Sensor_Interface_t (обов'язково).
 */
typedef struct {
    Position_Sensor_Interface_t interface; /**< Інтерфейс датчика (ПЕРШИЙ!) */

    uint32_t cpr;                          /**< Counts per revolution (після x4) */

    volatile int32_t  count;              /**< Позиція — оновлюється в EXTI ISR */
    volatile uint32_t period_us;          /**< Період між імпульсами A (мкс) — з IC ISR */
    volatile uint32_t last_pulse_ms;      /**< Час останнього імпульсу (для zero-velocity) */
    volatile int8_t   direction;          /**< +1 або -1 — напрямок з X4 таблиці */

    uint8_t enc_state;                    /**< Поточний стан X4 автомату (last AB) */
} Incremental_Encoder_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера
 *
 * Прив'язує callbacks до interface. Після виклику:
 * Position_Sensor_Init(&driver->interface, multi_turn).
 *
 * @param driver Вказівник на структуру драйвера
 * @param cpr    Counts per revolution (x4 — усі 4 фронти обох каналів)
 */
Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t* driver,
                                           uint32_t cpr);

/**
 * @brief EXTI ISR handler — викликати з board ISR при зміні каналу A або B
 *
 * @param driver Вказівник на драйвер
 * @param pin_a  Поточний стан каналу A (0 або 1)
 * @param pin_b  Поточний стан каналу B (0 або 1)
 */
void Incremental_Encoder_EXTI_Handler(Incremental_Encoder_Driver_t* driver,
                                       uint8_t pin_a, uint8_t pin_b);

/**
 * @brief IC ISR handler — викликати з board TIM Input Capture ISR
 *
 * @param driver    Вказівник на драйвер
 * @param period_us Виміряний період між наростаючими фронтами A (мкс)
 */
void Incremental_Encoder_IC_Handler(Incremental_Encoder_Driver_t* driver,
                                     uint32_t period_us);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_INCREMENTAL_ENCODER_H */
