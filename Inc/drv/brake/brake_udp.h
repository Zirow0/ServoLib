/**
 * @file brake_udp.h
 * @brief Драйвер електронних гальм для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить драйвер електронних гальм для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

#ifndef SERVOCORE_DRV_BRAKE_UDP_H
#define SERVOCORE_DRV_BRAKE_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../hwd/hwd_udp.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація UDP гальмового драйвера
 */
typedef struct {
    uint32_t timeout_ms;              /**< Таймаут UDP (мс) */
    uint32_t update_interval_ms;      /**< Інтервал оновлення (мс) */
    bool auto_engage_on_error;        /**< Автоматичне блокування при помилці */
    uint32_t error_threshold;         /**< Поріг помилок перед відключенням */
} Brake_UDP_Config_t;

/**
 * @brief Структура UDP гальмового драйвера
 */
typedef struct {
    /* Конфігурація */
    Brake_UDP_Config_t config;        /**< Конфігураційні параметри */
    
    /* Стан */
    UDP_Brake_Cmd_t cmd;              /**< Команда для відправки */
    UDP_Brake_State_t state;          /**< Стан від моделі */
    bool current_engaged;             /**< Поточний стан гальм */
    
    /* Таймінги */
    uint32_t last_update_ms;          /**< Час останнього оновлення */
    uint32_t last_cmd_ms;             /**< Час останньої команди */
    
    /* Помилки */
    uint32_t error_count;             /**< Кількість помилок */
    uint32_t comm_error_count;        /**< Кількість помилок зв'язку */
    
    /* Прапори */
    bool initialized;                 /**< Прапорець ініціалізації */
    bool enabled;                     /**< Драйвер увімкнений */
    bool emergency_engaged;           /**< Аварійне блокування активоване */
} Brake_UDP_Driver_t;

/* Exported constants --------------------------------------------------------*/

#define BRAKE_UDP_DEFAULT_TIMEOUT_MS      100    /**< Таймаут за замовчуванням */
#define BRAKE_UDP_DEFAULT_UPDATE_MS       10     /**< Інтервал оновлення за замовчуванням */
#define BRAKE_UDP_ERROR_THRESHOLD         10     /**< Поріг помилок */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення UDP гальмового драйвера
 * 
 * @param brake Вказівник на драйвер
 * @param config Конфігурація драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_Create(Brake_UDP_Driver_t* brake, 
                                const Brake_UDP_Config_t* config);

/**
 * @brief Ініціалізація UDP гальмового драйвера
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_Init(Brake_UDP_Driver_t* brake);

/**
 * @brief Відпущення гальм
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_Release(Brake_UDP_Driver_t* brake);

/**
 * @brief Активація гальм
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_Engage(Brake_UDP_Driver_t* brake);

/**
 * @brief Аварійна активація гальм
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_EmergencyEngage(Brake_UDP_Driver_t* brake);

/**
 * @brief Оновлення стану гальм (викликати періодично)
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_Update(Brake_UDP_Driver_t* brake);

/**
 * @brief Отримання поточного стану гальм
 * 
 * @param brake Вказівник на драйвер
 * @return bool Поточний стан гальм (true = активні)
 */
bool Brake_UDP_IsEngaged(const Brake_UDP_Driver_t* brake);

/**
 * @brief Скидання лічильника помилок
 * 
 * @param brake Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Brake_UDP_ResetErrorCount(Brake_UDP_Driver_t* brake);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_BRAKE_UDP_H */