/**
 * @file current.h
 * @brief Універсальний інтерфейс драйвера датчика струму
 * @author ServoCore Team
 * @date 2025
 *
 * Послідовність ініціалізації:
 *   1. XxxDriver_Create(&driver, &config)
 *   2. Current_Sensor_Calibrate(&driver.interface) — при нульовому струмі
 *   3. while(1) { Current_Sensor_Update(&driver.interface); }
 */

#ifndef SERVOCORE_DRV_CURRENT_H
#define SERVOCORE_DRV_CURRENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Сирі дані з датчика (заповнює драйвер у read_raw)
 *
 * Драйвер конвертує напругу АЦП у Ампери.
 * Зміщення нуля та фільтрація — відповідальність current.c.
 */
typedef struct {
    float current_a;  /**< Миттєвий струм (А), без корекції та фільтрації */
    bool  valid;      /**< Валідність даних */
} Current_Raw_Data_t;

/**
 * @brief Параметри ініціалізації датчика струму
 */
typedef struct {
    float overcurrent_threshold_a;  /**< Поріг перевантаження (А), 0.0 = вимкнено */
    float ema_alpha;                 /**< Коефіцієнт EMA фільтра [0.0 - 1.0],
                                         вищий = менше фільтрації, швидша реакція */
} Current_Params_t;

/**
 * @brief Внутрішні дані датчика (керується current.c)
 */
typedef struct {
    float ema_alpha;
    float overcurrent_threshold_a;

    float filtered_current_a;  /**< Відфільтрований струм (А) */
    float zero_offset_a;       /**< Зміщення нуля (А), визначається калібруванням */
    float peak_current_a;      /**< Абсолютний пік (А) */

    bool  overcurrent_flag;    /**< Прапорець перевантаження */
} Current_Sensor_Data_t;

/**
 * @brief Hardware callbacks — надаються конкретним драйвером
 */
typedef struct {
    /** @brief Ініціалізація апаратури (перевірка АЦП) */
    Servo_Status_t (*init)(void* driver_data, const Current_Params_t* params);

    /**
     * @brief Зчитування миттєвого струму з апаратури
     *
     * Конвертує напругу АЦП у Ампери. НЕ виконує фільтрацію та
     * НЕ застосовує зміщення нуля.
     */
    Servo_Status_t (*read_raw)(void* driver_data, Current_Raw_Data_t* raw);
} Current_Sensor_HW_Callbacks_t;

/**
 * @brief Універсальний інтерфейс датчика струму
 *
 * Конкретний драйвер (напр. ACS712_Driver_t) має цю структуру ПЕРШИМ полем,
 * що дозволяє безпечний каст: (Current_Sensor_Interface_t*)&acs712_driver.
 */
typedef struct Current_Sensor_Interface Current_Sensor_Interface_t;

struct Current_Sensor_Interface {
    Current_Sensor_Data_t         data;         /**< Логіка, стан */
    Current_Sensor_HW_Callbacks_t hw;           /**< Апаратні операції */
    void*                         driver_data;  /**< Вказівник на конкретний драйвер */
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація датчика струму
 *
 * Викликає hw.init(), зберігає параметри, обнуляє дані.
 *
 * @param sensor Вказівник на інтерфейс
 * @param params Параметри (поріг перевантаження, коефіцієнт EMA)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_Init(Current_Sensor_Interface_t* sensor,
                                    const Current_Params_t* params);

/**
 * @brief Оновлення датчика струму
 *
 * Викликати у контурному циклі (1 кГц).
 * Послідовність: read_raw → корекція нуля → EMA → пік → захист.
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_Update(Current_Sensor_Interface_t* sensor);

/**
 * @brief Калібрування нульового зміщення
 *
 * КРИТИЧНО: викликати при нульовому струмі (двигун зупинений, ШІМ вимкнений).
 * Усереднює read_raw за ~50 мс, зберігає zero_offset_a.
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_Calibrate(Current_Sensor_Interface_t* sensor);

/**
 * @brief Отримання відфільтрованого струму
 *
 * @param sensor     Вказівник на інтерфейс
 * @param current_a  Результат у Амперах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_GetCurrent(const Current_Sensor_Interface_t* sensor,
                                          float* current_a);

/**
 * @brief Отримання абсолютного піку з моменту старту або останнього скидання
 *
 * @param sensor  Вказівник на інтерфейс
 * @param peak_a  Пік у Амперах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_GetPeakCurrent(const Current_Sensor_Interface_t* sensor,
                                               float* peak_a);

/**
 * @brief Перевірка прапорця перевантаження
 *
 * @param sensor Вказівник на інтерфейс
 * @return bool true якщо зафіксовано перевантаження
 */
bool Current_Sensor_IsOvercurrent(const Current_Sensor_Interface_t* sensor);

/**
 * @brief Скидання прапорця перевантаження та пікового значення
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_ResetPeak(Current_Sensor_Interface_t* sensor);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_CURRENT_H */
