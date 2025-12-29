/**
 * @file hwd_udp.h
 * @brief Апаратна абстракція UDP для емуляції
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить інтерфейс апаратної абстракції для UDP комунікації
 * при емуляції сервоприводу на ПК.
 */

#ifndef SERVOCORE_HWD_UDP_H
#define SERVOCORE_HWD_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Типи UDP повідомлень
 */
typedef enum {
    UDP_MSG_TYPE_MOTOR_CMD    = 0x01,  /**< Команда двигуну */
    UDP_MSG_TYPE_SENSOR_CMD   = 0x02,  /**< Команда сенсору */
    UDP_MSG_TYPE_BRAKE_CMD    = 0x03,  /**< Команда гальмам */
    UDP_MSG_TYPE_MOTOR_STATE  = 0x10,  /**< Стан двигуна від моделі */
    UDP_MSG_TYPE_SENSOR_STATE = 0x11,  /**< Стан сенсора від моделі */
    UDP_MSG_TYPE_BRAKE_STATE  = 0x12,  /**< Стан гальм від моделі */
    UDP_MSG_TYPE_CONFIG       = 0x20,  /**< Команда конфігурації */
    UDP_MSG_TYPE_PING         = 0xFF   /**< Перевірка зв'язку */
} UDP_MsgType_t;

/**
 * @brief Структура UDP повідомлення
 */
typedef struct {
    UDP_MsgType_t msg_type;       /**< Тип повідомлення */
    uint32_t sequence_number;     /**< Номер послідовності */
    uint8_t payload[256];         /**< Тіло повідомлення */
    uint16_t payload_size;        /**< Розмір тіла повідомлення */
} UDP_Message_t;

/**
 * @brief Команда для двигуна
 */
typedef struct {
    float power;                  /**< Потужність (-100.0 до +100.0) */
    float target_position;        /**< Цільова позиція (градуси) */
    float target_velocity;        /**< Цільова швидкість (град/с) */
} UDP_Motor_Cmd_t;

/**
 * @brief Стан двигуна від моделі
 */
typedef struct {
    float position;               /**< Поточна позиція (градуси) */
    float velocity;               /**< Поточна швидкість (град/с) */
    float current;                /**< Поточний струм (mA) */
    bool stalled;                 /**< Чи заклинений двигун */
    bool overcurrent;             /**< Чи перевищено струм */
    uint32_t error_flags;         /**< Прапори помилок */
} UDP_Motor_State_t;

/**
 * @brief Команда для гальм
 */
typedef struct {
    bool engaged;                 /**< Активувати/деактивувати гальма */
    uint32_t timestamp;           /**< Час відправлення (для синхронізації) */
} UDP_Brake_Cmd_t;

/**
 * @brief Стан гальм від моделі
 */
typedef struct {
    bool engaged;                 /**< Чи гальма активні */
    bool ready;                   /**< Чи гальма готові */
    uint32_t error_flags;         /**< Прапори помилок */
} UDP_Brake_State_t;

/**
 * @brief Команда для сенсора
 */
typedef struct {
    uint8_t request_type;         /**< Тип запиту */
} UDP_Sensor_Cmd_t;

/**
 * @brief Стан сенсора від моделі
 */
typedef struct {
    float angle;                  /**< Кут (градуси) */
    float velocity;               /**< Швидкість (град/с) */
    bool connected;               /**< Чи сенсор підключено */
    uint32_t error_flags;         /**< Прапори помилок */
} UDP_Sensor_State_t;

/* Exported constants --------------------------------------------------------*/

#ifndef UDP_TIMEOUT_MS
#define UDP_TIMEOUT_MS              100                /**< Таймаут UDP (мс) */
#endif

#ifndef UDP_SERVER_IP
#define UDP_SERVER_IP               "127.0.0.1"        /**< IP адреса сервера моделі */
#endif

#ifndef UDP_SERVER_PORT
#define UDP_SERVER_PORT             8888               /**< Порт сервера моделі */
#endif

#ifndef UDP_CLIENT_PORT
#define UDP_CLIENT_PORT             8889               /**< Порт клієнта емулятора */
#endif

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація UDP зв'язку
 *
 * @param server_ip IP адреса сервера моделі
 * @param server_port Порт сервера моделі
 * @param client_port Порт клієнта емулятора
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UDP_Init(const char* server_ip, uint16_t server_port, uint16_t client_port);

/**
 * @brief Відправлення UDP повідомлення
 *
 * @param msg Вказівник на повідомлення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UDP_Send(const UDP_Message_t* msg);

/**
 * @brief Отримання UDP повідомлення
 *
 * @param msg Вказівник на буфер для повідомлення
 * @param timeout_ms Таймаут (мс)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UDP_Receive(UDP_Message_t* msg, uint32_t timeout_ms);

/**
 * @brief Деініціалізація UDP зв'язку
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UDP_DeInit(void);

/**
 * @brief Перевірка зв'язку з сервером
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_UDP_Ping(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_UDP_H */
