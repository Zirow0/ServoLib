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
 * @brief Апаратна конфігурація енкодера
 *
 * Усі значення — апаратно-специфічні константи (GPIO*, TIM*, AF*).
 * Зберігаються у драйвері та використовуються в ISR dispatch.
 *
 * Канал A → TIM Input Capture (AF) + EXTI (обидва фронти)
 * Канал B → GPIO input + EXTI (обидва фронти)
 */
typedef struct {
    uint32_t gpio_port_a;  /**< GPIO порт каналу A (напр. GPIOA) */
    uint16_t gpio_pin_a;   /**< GPIO пін каналу A (напр. GPIO0) */
    uint8_t  gpio_af_a;    /**< AF номер для TIM IC на піні A (напр. GPIO_AF1) */
    uint32_t gpio_port_b;  /**< GPIO порт каналу B */
    uint16_t gpio_pin_b;   /**< GPIO пін каналу B */
    uint32_t timer_base;   /**< База таймера (напр. TIM2) */
    uint32_t timer_rcc;    /**< RCC таймера (напр. RCC_TIM2) */
    uint32_t ic_channel;   /**< IC канал (0-based = enum tim_ic_id): 0=CH1, 1=CH2, 2=CH3, 3=CH4 */
} Incremental_Encoder_HW_t;

/**
 * @brief Структура драйвера інкрементального енкодера
 *
 * Перше поле — Position_Sensor_Interface_t (обов'язково).
 */
typedef struct {
    Position_Sensor_Interface_t interface; /**< Інтерфейс датчика (ПЕРШИЙ!) */

    uint32_t                    cpr;       /**< Counts per revolution (після x4) */
    Incremental_Encoder_HW_t   hw;        /**< Апаратна конфігурація (для ISR dispatch) */

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
 * Зберігає HW конфіг та прив'язує callbacks до interface.
 * Апаратна ініціалізація (EXTI, TIM IC) відбувається при виклику
 * Position_Sensor_Init(&driver->interface).
 *
 * @param driver Вказівник на структуру драйвера
 * @param cpr    Counts per revolution (x4 — усі 4 фронти обох каналів)
 * @param hw     Апаратна конфігурація пінів та таймера
 */
Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t *driver,
                                           uint32_t cpr,
                                           const Incremental_Encoder_HW_t *hw);

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
