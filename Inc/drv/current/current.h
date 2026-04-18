/**
 * @file current.h
 * @brief Універсальний інтерфейс драйвера датчика струму
 * @author ServoCore Team
 * @date 2025
 *
 * Hardware Callbacks Pattern: спільна логіка (фільтрація EMA, відстеження піку,
 * виявлення перевантаження, калібрування нуля) в current.c.
 * Апаратна специфіка (АЦП читання, конвертація) — у конкретних драйверах.
 *
 * Послідовність ініціалізації:
 *   1. XxxDriver_Create(&driver, &config)   — factory, заповнює hw callbacks
 *   2. Current_Sensor_Init(&driver.interface, &params)
 *   3. Current_Sensor_Calibrate(&driver.interface) — при нульовому струмі
 *   4. while(1) { Current_Sensor_Update(&driver.interface); }
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
 * @brief Можливості датчика струму
 */
typedef enum {
    CURRENT_CAP_BIDIRECTIONAL  = 0x01,  /**< Вимірює струм в обох напрямках */
    CURRENT_CAP_UNIDIRECTIONAL = 0x02,  /**< Тільки додатний струм */
    CURRENT_CAP_OVERCURRENT_HW = 0x04,  /**< Апаратний компаратор перевантаження */
} Current_Capabilities_t;

/**
 * @brief Напрямок струму
 */
typedef enum {
    CURRENT_DIR_ZERO     = 0,  /**< Струм у мертвій зоні */
    CURRENT_DIR_POSITIVE = 1,  /**< Прямий напрямок */
    CURRENT_DIR_NEGATIVE = 2,  /**< Зворотний напрямок */
} Current_Direction_t;

/**
 * @brief Сирі дані з датчика (заповнює драйвер у read_raw)
 *
 * Драйвер конвертує апаратні дані (напруга АЦП) у Ампери.
 * Зміщення нуля та фільтрація — відповідальність current.c.
 */
typedef struct {
    float    current_a;      /**< Миттєвий струм (А), без корекції та фільтрації */
    uint32_t timestamp_us;   /**< Мітка часу зчитування (мікросекунди) */
    bool     valid;          /**< Валідність даних */
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
 * @brief Статистика датчика струму
 */
typedef struct {
    uint32_t update_count;       /**< Кількість успішних оновлень */
    uint32_t error_count;        /**< Кількість помилок зчитування */
    uint32_t overcurrent_count;  /**< Кількість подій перевантаження */
    float    last_current_a;     /**< Останнє виміряне значення (після корекції) */
    float    peak_current_a;     /**< Абсолютний пік з моменту старту або скидання */
    bool     is_calibrated;      /**< Стан калібрування */
} Current_Stats_t;

/**
 * @brief Внутрішні дані датчика (керується current.c)
 */
typedef struct {
    /* Параметри обробки (зберігаються з Current_Params_t при Init) */
    float    ema_alpha;
    float    overcurrent_threshold_a;

    /* Оброблені дані */
    float    filtered_current_a;  /**< Відфільтрований струм (А) */
    float    zero_offset_a;       /**< Зміщення нуля (А), визначається калібруванням */
    float    peak_current_a;      /**< Абсолютний пік (А) */

    /* Стан */
    bool                overcurrent_flag;  /**< Прапорець перевантаження */
    Current_Direction_t direction;

    /* Статистика */
    Current_Stats_t stats;

    /* Службові */
    bool          is_initialized;
    bool          is_calibrated;
    Servo_Error_t last_error;
} Current_Sensor_Data_t;

/**
 * @brief Hardware callbacks — надаються конкретним драйвером
 */
typedef struct {
    /** @brief Ініціалізація апаратури (АЦП, GPIO) */
    Servo_Status_t (*init)(void* driver_data, const Current_Params_t* params);

    /** @brief Деініціалізація апаратури */
    Servo_Status_t (*deinit)(void* driver_data);

    /**
     * @brief Зчитування миттєвого струму з апаратури
     *
     * Драйвер конвертує напругу АЦП у Ампери через коефіцієнт датчика.
     * НЕ виконує фільтрацію та НЕ застосовує зміщення нуля.
     */
    Servo_Status_t (*read_raw)(void* driver_data, Current_Raw_Data_t* raw);

    /**
     * @brief Апаратне калібрування (опціонально, NULL якщо не підтримується)
     *
     * Для датчиків з вбудованим авто-нулем. Якщо NULL, калібрування
     * виконується програмно в Current_Sensor_Calibrate() через read_raw.
     */
    Servo_Status_t (*calibrate)(void* driver_data);
} Current_Sensor_HW_Callbacks_t;

/**
 * @brief Універсальний інтерфейс датчика струму
 *
 * Конкретний драйвер (напр. ACS712_Driver_t) має цю структуру ПЕРШИМ полем,
 * що дозволяє безпечний каст: (Current_Sensor_Interface_t*)&acs712_driver.
 */
typedef struct Current_Sensor_Interface Current_Sensor_Interface_t;

struct Current_Sensor_Interface {
    Current_Sensor_Data_t         data;          /**< Логіка, стан, статистика */
    Current_Sensor_HW_Callbacks_t hw;            /**< Апаратні операції */
    Current_Capabilities_t        capabilities;  /**< Можливості датчика */
    void*                         driver_data;   /**< Вказівник на конкретний драйвер */
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
 * @brief Деініціалізація датчика струму
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_DeInit(Current_Sensor_Interface_t* sensor);

/**
 * @brief Оновлення датчика струму
 *
 * Викликати у контурному циклі (1 кГц).
 * Послідовність: read_raw → корекція нуля → EMA → пік → захист → напрямок.
 *
 * @param sensor Вказівник на інтерфейс
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_Update(Current_Sensor_Interface_t* sensor);

/**
 * @brief Калібрування нульового зміщення
 *
 * КРИТИЧНО: викликати при нульовому струмі (двигун зупинений, ШІМ вимкнений).
 * Якщо hw.calibrate == NULL: усереднює read_raw за ~50 мс, зберігає zero_offset_a.
 * Якщо hw.calibrate != NULL: делегує апаратному калібруванню.
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

/**
 * @brief Отримання статистики
 *
 * @param sensor Вказівник на інтерфейс
 * @param stats  Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Current_Sensor_GetStats(const Current_Sensor_Interface_t* sensor,
                                        Current_Stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_CURRENT_H */
