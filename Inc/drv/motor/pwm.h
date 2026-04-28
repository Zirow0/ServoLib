/**
 * @file pwm.h
 * @brief PWM драйвер DC двигуна
 *
 * Підтримує одноканальне (PWM + DIR) та двоканальне (H-bridge) керування.
 */

#ifndef SERVOCORE_DRV_MOTOR_PWM_H
#define SERVOCORE_DRV_MOTOR_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "../../drv/motor/motor.h"
#include "../../hwd/hwd_pwm.h"
#include "../../hwd/hwd_gpio.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Тип керування PWM мотором
 */
typedef enum {
    PWM_MOTOR_TYPE_SINGLE_PWM_DIR = 0,  /**< Один PWM + GPIO напрямок */
    PWM_MOTOR_TYPE_DUAL_PWM       = 1   /**< Два PWM каналу (H-bridge) */
} PWM_Motor_Type_t;

/**
 * @brief Конфігурація PWM двигуна
 */
typedef struct {
    PWM_Motor_Type_t  type;      /**< Тип керування */
    HWD_PWM_Handle_t* pwm_fwd;  /**< PWM канал для прямого ходу */
    HWD_PWM_Handle_t* pwm_bwd;  /**< PWM канал для зворотного ходу (або NULL) */
    void*             gpio_dir; /**< GPIO порт для напрямку (опціонально) */
    uint32_t          gpio_pin; /**< Пін GPIO для напрямку */
} PWM_Motor_Config_t;

/**
 * @brief Структура PWM драйвера двигуна
 */
typedef struct {
    Motor_Interface_t  interface; /**< Інтерфейс двигуна (перше поле — обов'язково) */
    PWM_Motor_Config_t config;    /**< Конфігурація PWM */
    HWD_GPIO_Pin_t     gpio_dir;  /**< HWD дескриптор GPIO напрямку */
} PWM_Motor_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення PWM драйвера двигуна
 *
 * Заповнює hardware callbacks в interface. Після виклику передайте
 * &driver->interface до Motor_Init().
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Вказівник на конфігурацію PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                const PWM_Motor_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_MOTOR_PWM_H */
