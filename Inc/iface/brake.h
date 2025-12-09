/**
 * @file brake.h
 * @brief Інтерфейс взаємодії з драйвером гальм
 * @author ServoCore Team
 * @date 2025
 *
 * Високорівневий API для керування гальмами сервоприводу
 */

#ifndef SERVOCORE_IFACE_BRAKE_H
#define SERVOCORE_IFACE_BRAKE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../drv/brake/brake.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Параметри ініціалізації гальм
 */
typedef struct {
    /* GPIO конфігурація */
    void* gpio_port;            /**< Порт GPIO (GPIO_TypeDef*) */
    uint16_t gpio_pin;          /**< Номер піна */
    bool active_high;           /**< Логіка активації */

    /* Таймінги (мс) */
    uint32_t release_delay;     /**< Затримка перед відпусканням */
    uint32_t engage_timeout;    /**< Таймаут бездіяльності */
} BrakeInterface_InitParams_t;

/**
 * @brief Статус гальм
 */
typedef struct {
    Brake_State_t state;        /**< Поточний стан */
    Brake_Mode_t mode;          /**< Режим роботи */
    uint32_t idle_time_ms;      /**< Зарезервовано (не використовується) */
    bool is_initialized;        /**< Чи ініціалізовано */
} BrakeInterface_Status_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація системи гальм
 *
 * @param params Параметри ініціалізації
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_Init(const BrakeInterface_InitParams_t* params);

/**
 * @brief Деініціалізація системи гальм
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_Deinit(void);

/**
 * @brief Відпустити гальма для руху
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_Release(void);

/**
 * @brief Активувати гальма (зупинка)
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_Engage(void);

/**
 * @brief Оновлення стану (викликати в головному циклі)
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_Update(void);

/**
 * @brief Повідомити про активність сервоприводу
 *
 * Викликається при будь-якій команді або русі
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_NotifyActivity(void);

/**
 * @brief Встановити режим роботи гальм
 *
 * @param mode Режим роботи (MANUAL/AUTO)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_SetMode(Brake_Mode_t mode);

/**
 * @brief Отримати режим роботи
 *
 * @return Brake_Mode_t Поточний режим
 */
Brake_Mode_t BrakeInterface_GetMode(void);

/**
 * @brief Отримати стан гальм
 *
 * @return Brake_State_t Поточний стан
 */
Brake_State_t BrakeInterface_GetState(void);

/**
 * @brief Отримати повний статус системи гальм
 *
 * @param status Вказівник на структуру статусу
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_GetStatus(BrakeInterface_Status_t* status);

/**
 * @brief Перевірка чи гальма активні
 *
 * @return bool true якщо гальма активні
 */
bool BrakeInterface_IsEngaged(void);

/**
 * @brief Перевірка чи гальма відпущені
 *
 * @return bool true якщо гальма відпущені
 */
bool BrakeInterface_IsReleased(void);

/**
 * @brief Аварійна зупинка (негайна активація гальм)
 *
 * Використовується у критичних ситуаціях
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_EmergencyStop(void);

/**
 * @brief Налаштування таймінгів гальм
 *
 * @param release_delay_ms Затримка перед відпусканням (мс)
 * @param engage_timeout_ms Таймаут бездіяльності (мс)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t BrakeInterface_ConfigureTimings(uint32_t release_delay_ms,
                                                uint32_t engage_timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_IFACE_BRAKE_H */
