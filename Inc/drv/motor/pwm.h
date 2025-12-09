/**
 * @file pwm.h
 * @brief PWM драйвер двигуна
 * @author ServoCore Team
 * @date 2025
 *
 * Драйвер для керування DC двигуном через PWM сигнали.
 * Підтримує одноканальне та двоканальне керування (H-bridge).
 */

#ifndef SERVOCORE_DRV_MOTOR_PWM_H
#define SERVOCORE_DRV_MOTOR_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../iface/motor.h"
#include "./base.h"
#include "../../hwd/hwd_pwm.h"
#include "../../hwd/hwd_gpio.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Тип керування PWM мотором
 */
typedef enum {
    PWM_MOTOR_TYPE_SINGLE_PWM_DIR = 0,  /**< Один PWM + GPIO напрямок */
    PWM_MOTOR_TYPE_DUAL_PWM       = 1,  /**< Два PWM каналу (H-bridge) */
    PWM_MOTOR_DUAL_PWM            = 1,  /**< Два PWM каналу (alias) */
    PWM_MOTOR_TYPE_SIGN_MAGNITUDE = 2   /**< PWM + знак */
} PWM_Motor_Type_t;

/**
 * @brief Конфігурація PWM двигуна
 */
typedef struct {
    PWM_Motor_Type_t type;       /**< Тип керування */
    HWD_PWM_Handle_t* pwm_fwd;   /**< PWM канал для прямого ходу */
    HWD_PWM_Handle_t* pwm_bwd;   /**< PWM канал для зворотного ходу (або NULL) */
    void* gpio_dir;              /**< GPIO для напрямку (опціонально) */
    uint32_t gpio_pin;           /**< Пін GPIO для напрямку */
} PWM_Motor_Config_t;

/**
 * @brief Структура PWM драйвера двигуна
 */
typedef struct {
    Motor_Interface_t interface; /**< Інтерфейс двигуна */
    Motor_Base_Data_t base;      /**< Базові дані */
    PWM_Motor_Config_t config;   /**< Конфігурація PWM */

    HWD_GPIO_Pin_t gpio_dir_pin; /**< HWD дескриптор для GPIO напрямку */
    float current_duty_percent;  /**< Поточний duty cycle (%) */
    bool is_braking;             /**< Прапорець гальмування */
} PWM_Motor_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення PWM драйвера двигуна
 *
 * Ініціалізує структуру інтерфейсу з функціями PWM драйвера
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Вказівник на конфігурацію PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                const PWM_Motor_Config_t* config);

/**
 * @brief Ініціалізація PWM драйвера
 *
 * @param driver Вказівник на структуру драйвера
 * @param params Параметри двигуна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_Init(PWM_Motor_Driver_t* driver,
                              const Motor_Params_t* params);

/**
 * @brief Деініціалізація PWM драйвера
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_DeInit(PWM_Motor_Driver_t* driver);

/**
 * @brief Встановлення потужності PWM двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @param power Потужність (-100.0 до +100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_SetPower(PWM_Motor_Driver_t* driver, float power);

/**
 * @brief Зупинка PWM двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_Stop(PWM_Motor_Driver_t* driver);

/**
 * @brief Аварійна зупинка PWM двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_EmergencyStop(PWM_Motor_Driver_t* driver);

/**
 * @brief Встановлення напрямку обертання
 *
 * @param driver Вказівник на структуру драйвера
 * @param direction Напрямок обертання
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_SetDirection(PWM_Motor_Driver_t* driver,
                                      Motor_Direction_t direction);

/**
 * @brief Отримання стану двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @param state Вказівник для збереження стану
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_GetState(PWM_Motor_Driver_t* driver,
                                  Motor_State_t* state);

/**
 * @brief Отримання статистики двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @param stats Вказівник для збереження статистики
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_GetStats(PWM_Motor_Driver_t* driver,
                                  Motor_Stats_t* stats);

/**
 * @brief Оновлення стану PWM двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_Update(PWM_Motor_Driver_t* driver);

/**
 * @brief Отримання інтерфейсу двигуна
 *
 * @param driver Вказівник на структуру драйвера
 * @return Motor_Interface_t* Вказівник на інтерфейс
 */
Motor_Interface_t* PWM_Motor_GetInterface(PWM_Motor_Driver_t* driver);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_PWM_H */
