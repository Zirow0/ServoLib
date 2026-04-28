/**
 * @file brake.h
 * @brief Універсальний інтерфейс драйвера гальм з hardware callbacks
 * @author ServoCore Team
 * @date 2025
 */

#ifndef SERVOCORE_DRV_BRAKE_H
#define SERVOCORE_DRV_BRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан гальм (4 стани з перехідними)
 */
typedef enum {
    BRAKE_STATE_ENGAGED = 0,    /**< Стабільний стан: гальма активні (блокують рух) */
    BRAKE_STATE_RELEASED = 1,   /**< Стабільний стан: гальма відпущені (дозволяють рух) */
    BRAKE_STATE_ENGAGING = 2,   /**< Перехідний стан: процес активації (releasing → engaged) */
    BRAKE_STATE_RELEASING = 3   /**< Перехідний стан: процес відпускання (engaged → released) */
} Brake_State_t;

/**
 * @brief Параметри ініціалізації базового інтерфейсу
 */
typedef struct {
    uint32_t engage_time_ms;    /**< Час переходу RELEASING → ENGAGED (мс) */
    uint32_t release_time_ms;   /**< Час переходу ENGAGING → RELEASED (мс) */
} Brake_Params_t;

/**
 * @brief Дані базового інтерфейсу (логіка станів)
 */
typedef struct {
    Brake_State_t state;                /**< Поточний стан */
    uint32_t transition_start_time_ms;  /**< Час початку перехідного стану */
    uint32_t engage_time_ms;            /**< Таймінг переходу до ENGAGED */
    uint32_t release_time_ms;           /**< Таймінг переходу до RELEASED */
} Brake_Data_t;

/* Forward declaration */
struct Brake_Interface_s;

/**
 * @brief Hardware callbacks для апаратних операцій
 */
typedef struct {
    Servo_Status_t (*init)(void* driver_data);     /**< Ініціалізація апаратури */
    Servo_Status_t (*engage)(void* driver_data);   /**< Фізична активація гальм */
    Servo_Status_t (*release)(void* driver_data);  /**< Фізичне відпускання гальм */
} Brake_Hardware_Callbacks_t;

/**
 * @brief Універсальний інтерфейс драйвера гальм
 */
typedef struct Brake_Interface_s {
    Brake_Data_t data;                      /**< Базова логіка (стани, таймінги) */
    Brake_Hardware_Callbacks_t hw;          /**< Апаратні операції (callbacks) */
    void* driver_data;                      /**< Вказівник на конкретний драйвер */
} Brake_Interface_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація драйвера гальм
 *
 * Викликає hw.init() та встановлює початковий стан ENGAGED (fail-safe)
 *
 * @param brake Вказівник на інтерфейс
 * @param params Параметри ініціалізації (таймінги)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Init(Brake_Interface_t* brake, const Brake_Params_t* params);

/**
 * @brief Активувати гальма (заблокувати рух)
 *
 * Викликає hw.engage() та переводить у стан ENGAGING
 *
 * @param brake Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Engage(Brake_Interface_t* brake);

/**
 * @brief Відпустити гальма (дозволити рух)
 *
 * Викликає hw.release() та переводить у стан RELEASING
 *
 * @param brake Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Release(Brake_Interface_t* brake);

/**
 * @brief Оновлення стану гальм (обробка переходів)
 *
 * Викликати періодично в головному циклі.
 * Переводить з перехідних станів у стабільні після закінчення таймінгу:
 * - ENGAGING → ENGAGED (після engage_time_ms)
 * - RELEASING → RELEASED (після release_time_ms)
 *
 * @param brake Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Update(Brake_Interface_t* brake);

/**
 * @brief Отримати поточний стан гальм
 *
 * @param brake Вказівник на інтерфейс
 * @return Brake_State_t Поточний стан (ENGAGED/RELEASED/ENGAGING/RELEASING)
 */
Brake_State_t Brake_GetState(const Brake_Interface_t* brake);

/**
 * @brief Перевірка чи гальма активні (стабільний стан ENGAGED)
 *
 * @param brake Вказівник на інтерфейс
 * @return bool true якщо гальма у стані ENGAGED
 */
bool Brake_IsEngaged(const Brake_Interface_t* brake);

/**
 * @brief Перевірка чи гальма відпущені (стабільний стан RELEASED)
 *
 * @param brake Вказівник на інтерфейс
 * @return bool true якщо гальма у стані RELEASED
 */
bool Brake_IsReleased(const Brake_Interface_t* brake);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_BRAKE_H */
