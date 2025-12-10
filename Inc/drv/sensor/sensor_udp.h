/**
 * @file sensor_udp.h
 * @brief Драйвер сенсора для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить драйвер сенсора для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

#ifndef SERVOCORE_DRV_SENSOR_UDP_H
#define SERVOCORE_DRV_SENSOR_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../hwd/hwd_udp.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура UDP сенсорного драйвера
 */
typedef struct {
    /* Стан */
    UDP_Sensor_Cmd_t cmd;             /**< Команда для відправки */
    UDP_Sensor_State_t state;         /**< Стан від моделі */
    float last_angle;                 /**< Останній виміряний кут */
    float last_velocity;              /**< Остання виміряна швидкість */
    
    /* Таймінги */
    uint32_t last_update_ms;          /**< Час останнього оновлення */
    uint32_t last_request_ms;         /**< Час останнього запиту */
    uint32_t update_interval_ms;      /**< Інтервал оновлення (мс) */
    
    /* Помилки */
    uint32_t error_count;             /**< Кількість помилок */
    uint32_t comm_error_count;        /**< Кількість помилок зв'язку */
    
    /* Прапори */
    bool initialized;                 /**< Прапорець ініціалізації */
    bool enabled;                     /**< Драйвер увімкнений */
    bool connected;                   /**< Сенсор підключений */
    bool request_pending;             /**< Очікується відповідь на запит */
} Sensor_UDP_Driver_t;

/* Exported constants --------------------------------------------------------*/

#define SENSOR_UDP_DEFAULT_UPDATE_MS        10     /**< Інтервал оновлення за замовчуванням */
#define SENSOR_UDP_DEFAULT_TIMEOUT_MS       50     /**< Таймаут за замовчуванням */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення UDP сенсорного драйвера
 * 
 * @param sensor Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_Create(Sensor_UDP_Driver_t* sensor);

/**
 * @brief Ініціалізація UDP сенсорного драйвера
 * 
 * @param sensor Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_Init(Sensor_UDP_Driver_t* sensor);

/**
 * @brief Зчитування кута з сенсора
 * 
 * @param sensor Вказівник на драйвер
 * @param[out] angle Кут у градусах
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_ReadAngle(Sensor_UDP_Driver_t* sensor, float* angle);

/**
 * @brief Отримання швидкості з сенсора
 * 
 * @param sensor Вказівник на драйвер
 * @param[out] velocity Швидкість (град/с)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_GetVelocity(Sensor_UDP_Driver_t* sensor, float* velocity);

/**
 * @brief Оновлення драйвера (викликати періодично)
 * 
 * @param sensor Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_Update(Sensor_UDP_Driver_t* sensor);

/**
 * @brief Перевірка підключення сенсора
 * 
 * @param sensor Вказівник на драйвер
 * @return bool Чи сенсор підключений
 */
bool Sensor_UDP_IsConnected(const Sensor_UDP_Driver_t* sensor);

/**
 * @brief Отримання останнього статусу сенсора
 * 
 * @param sensor Вказівник на драйвер
 * @return UDP_Sensor_State_t Статус сенсора
 */
UDP_Sensor_State_t Sensor_UDP_GetLastState(const Sensor_UDP_Driver_t* sensor);

/**
 * @brief Скидання лічильника помилок
 * 
 * @param sensor Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Sensor_UDP_ResetErrorCount(Sensor_UDP_Driver_t* sensor);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_SENSOR_UDP_H */