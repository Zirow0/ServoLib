/**
 * @file pwm_udp.h
 * @brief Драйвер PWM двигуна для емуляції через UDP
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить драйвер PWM двигуна для емуляції на ПК
 * через UDP зв'язок з математичною моделлю двигуна.
 */

#ifndef SERVOCORE_DRV_MOTOR_PWM_UDP_H
#define SERVOCORE_DRV_MOTOR_PWM_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../hwd/hwd_udp.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Режим інтерфейсу для UDP двигуна
 */
typedef enum {
    PWM_MOTOR_INTERFACE_UDP = 0x00,  /**< UDP інтерфейс для емуляції */
    PWM_MOTOR_INTERFACE_REAL = 0x01  /**< Реальний інтерфейс (не для емуляції) */
} PWM_Motor_UDP_Interface_t;

/**
 * @brief Конфігурація UDP PWM драйвера
 */
typedef struct {
    Motor_Type_t type;                /**< Тип двигуна */
    PWM_Motor_UDP_Interface_t interface_mode; /**< Режим інтерфейсу */
    uint32_t timeout_ms;              /**< Таймаут UDP (мс) */
    uint32_t update_interval_ms;      /**< Інтервал оновлення (мс) */
} PWM_Motor_UDP_Config_t;

/**
 * @brief Структура UDP PWM драйвера
 */
typedef struct {
    /* Конфігурація */
    PWM_Motor_UDP_Config_t config;    /**< Конфігураційні параметри */
    Motor_Params_t params;            /**< Параметри двигуна */
    
    /* Стан */
    float current_power;              /**< Поточна потужність (-100.0 до +100.0) */
    UDP_Motor_Cmd_t cmd;              /**< Команда для відправки */
    UDP_Motor_State_t state;          /**< Стан від моделі */
    
    /* Таймінги */
    uint32_t last_update_ms;          /**< Час останнього оновлення */
    uint32_t last_cmd_ms;             /**< Час останньої команди */
    
    /* Помилки */
    uint32_t error_count;             /**< Кількість помилок */
    uint32_t comm_error_count;        /**< Кількість помилок зв'язку */
    
    /* Прапори */
    bool initialized;                 /**< Прапорець ініціалізації */
    bool enabled;                     /**< Драйвер увімкнений */
    bool direction_inverted;          /**< Інвертований напрямок */
} PWM_Motor_UDP_Driver_t;

/* Exported constants --------------------------------------------------------*/

#define PWM_MOTOR_UDP_DEFAULT_TIMEOUT_MS      100    /**< Таймаут за замовчуванням */
#define PWM_MOTOR_UDP_DEFAULT_UPDATE_MS       1      /**< Інтервал оновлення за замовчуванням */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення UDP PWM драйвера
 * 
 * @param driver Вказівник на драйвер
 * @param config Конфігурація драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_Create(PWM_Motor_UDP_Driver_t* driver,
                                    const PWM_Motor_UDP_Config_t* config);

/**
 * @brief Ініціалізація UDP PWM драйвера
 * 
 * @param driver Вказівник на драйвер
 * @param params Параметри двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_Init(PWM_Motor_UDP_Driver_t* driver,
                                  const Motor_Params_t* params);

/**
 * @brief Встановлення потужності двигуна
 * 
 * @param driver Вказівник на драйвер
 * @param power Потужність (-100.0 до +100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_SetPower(PWM_Motor_UDP_Driver_t* driver, float power);

/**
 * @brief Отримання поточного стану двигуна
 * 
 * @param driver Вказівник на драйвер
 * @param[out] power Поточна потужність
 * @param[out] position Поточна позиція
 * @param[out] velocity Поточна швидкість
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_GetState(PWM_Motor_UDP_Driver_t* driver,
                                      float* power, float* position, float* velocity);

/**
 * @brief Зупинка двигуна
 * 
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_Stop(PWM_Motor_UDP_Driver_t* driver);

/**
 * @brief Аварійна зупинка двигуна
 * 
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_EmergencyStop(PWM_Motor_UDP_Driver_t* driver);

/**
 * @brief Оновлення драйвера (викликати періодично)
 * 
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_Update(PWM_Motor_UDP_Driver_t* driver);

/**
 * @brief Отримання останнього статусу двигуна
 * 
 * @param driver Вказівник на драйвер
 * @return UDP_Motor_State_t Статус двигуна
 */
UDP_Motor_State_t PWM_Motor_UDP_GetLastState(const PWM_Motor_UDP_Driver_t* driver);

/**
 * @brief Скидання лічильника помилок
 * 
 * @param driver Вказівник на драйвер
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_UDP_ResetErrorCount(PWM_Motor_UDP_Driver_t* driver);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_PWM_UDP_H */