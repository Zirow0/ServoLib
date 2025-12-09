/**
 * @file brake.h
 * @brief Драйвер електронних гальм для сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Керування електромагнітними гальмами з fail-safe логікою
 */

#ifndef SERVOCORE_DRV_BRAKE_H
#define SERVOCORE_DRV_BRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../hwd/hwd_gpio.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан гальм
 */
typedef enum {
    BRAKE_STATE_ENGAGED = 0,    /**< Гальма активні (блокують рух) */
    BRAKE_STATE_RELEASED = 1    /**< Гальма відпущені (дозволяють рух) */
} Brake_State_t;

/**
 * @brief Режим роботи гальм
 */
typedef enum {
    BRAKE_MODE_MANUAL,          /**< Ручне керування */
    BRAKE_MODE_AUTO             /**< Автоматичне керування з таймером */
} Brake_Mode_t;

/**
 * @brief Конфігурація гальм
 */
typedef struct {
    void* gpio_port;            /**< Порт GPIO (GPIO_TypeDef*) */
    uint16_t gpio_pin;          /**< Номер піна */

    bool active_high;           /**< true = активний високий рівень, false = низький */

    /* Параметри автоматичного режиму */
    uint32_t release_delay_ms;  /**< Затримка перед відпусканням гальм (мс) */
    uint32_t engage_timeout_ms; /**< Таймаут бездіяльності перед блокуванням (мс) */
} Brake_Config_t;

/**
 * @brief Драйвер гальм
 */
typedef struct {
    /* Конфігурація */
    Brake_Config_t config;

    /* Стан */
    Brake_State_t state;
    Brake_Mode_t mode;

    /* Таймінги */
    uint32_t last_activity_ms;  /**< Час останньої активності */
    uint32_t release_time_ms;   /**< Час коли гальма були відпущені */

    /* Прапорці */
    bool is_initialized;
    bool pending_release;       /**< Очікування відпускання гальм */
} Brake_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація драйвера гальм
 *
 * @param brake Вказівник на драйвер
 * @param config Конфігурація
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Init(Brake_Driver_t* brake, const Brake_Config_t* config);

/**
 * @brief Відпустити гальма (дозволити рух)
 *
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Release(Brake_Driver_t* brake);

/**
 * @brief Активувати гальма (заблокувати рух)
 *
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Engage(Brake_Driver_t* brake);

/**
 * @brief Оновлення стану гальм (викликати періодично)
 *
 * Автоматично активує гальма після таймауту бездіяльності
 *
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_Update(Brake_Driver_t* brake);

/**
 * @brief Повідомити про активність (скидає таймер)
 *
 * Викликати при русі або командах
 *
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_NotifyActivity(Brake_Driver_t* brake);

/**
 * @brief Встановити режим роботи
 *
 * @param brake Вказівник на драйвер
 * @param mode Режим роботи
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_SetMode(Brake_Driver_t* brake, Brake_Mode_t mode);

/**
 * @brief Отримати поточний стан гальм
 *
 * @param brake Вказівник на драйвер
 * @return Brake_State_t Поточний стан
 */
Brake_State_t Brake_GetState(const Brake_Driver_t* brake);

/**
 * @brief Перевірка чи гальма активні
 *
 * @param brake Вказівник на драйвер
 * @return bool true якщо гальма активні
 */
bool Brake_IsEngaged(const Brake_Driver_t* brake);

/**
 * @brief Аварійна активація гальм
 *
 * Миттєво активує гальма незалежно від режиму
 *
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_EmergencyEngage(Brake_Driver_t* brake);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_BRAKE_H */
