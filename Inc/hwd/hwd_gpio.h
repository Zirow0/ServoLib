/**
 * @file hwd_gpio.h
 * @brief Апаратна абстракція GPIO
 * @author ServoCore Team
 * @date 2025
 *
 * Незалежний від платформи інтерфейс для роботи з GPIO
 */

#ifndef SERVOCORE_HWD_GPIO_H
#define SERVOCORE_HWD_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Стан GPIO піна
 */
typedef enum {
    HWD_GPIO_PIN_RESET = 0,  /**< Низький рівень */
    HWD_GPIO_PIN_SET   = 1   /**< Високий рівень */
} HWD_GPIO_PinState_t;

/**
 * @brief Режим GPIO піна
 */
typedef enum {
    HWD_GPIO_MODE_INPUT  = 0,  /**< Вхід */
    HWD_GPIO_MODE_OUTPUT = 1,  /**< Вихід */
    HWD_GPIO_MODE_AF     = 2,  /**< Альтернативна функція */
    HWD_GPIO_MODE_ANALOG = 3   /**< Аналоговий режим */
} HWD_GPIO_Mode_t;

/**
 * @brief Підтяжка GPIO піна
 */
typedef enum {
    HWD_GPIO_NOPULL   = 0,  /**< Без підтяжки */
    HWD_GPIO_PULLUP   = 1,  /**< Підтяжка до VCC */
    HWD_GPIO_PULLDOWN = 2   /**< Підтяжка до GND */
} HWD_GPIO_Pull_t;

/**
 * @brief Дескриптор GPIO піна
 */
typedef struct {
    void* port;          /**< Вказівник на порт (GPIO_TypeDef*) */
    uint16_t pin;        /**< Номер піна */
    HWD_GPIO_Mode_t mode;/**< Режим роботи */
    HWD_GPIO_Pull_t pull;/**< Підтяжка */
} HWD_GPIO_Pin_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Запис значення на GPIO пін
 *
 * @param port Вказівник на GPIO порт
 * @param pin Номер піна
 * @param state Стан піна (SET/RESET)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GPIO_WritePin(void* port, uint16_t pin, HWD_GPIO_PinState_t state);

/**
 * @brief Читання значення з GPIO піна
 *
 * @param pin Вказівник на дескриптор піна
 * @param state Вказівник для збереження стану
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GPIO_ReadPin(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t* state);

/**
 * @brief Перемикання стану GPIO піна
 *
 * @param pin Вказівник на дескриптор піна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GPIO_TogglePin(const HWD_GPIO_Pin_t* pin);

/**
 * @brief Ініціалізація GPIO піна
 *
 * @param pin Вказівник на дескриптор піна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GPIO_InitPin(const HWD_GPIO_Pin_t* pin);

/**
 * @brief Деініціалізація GPIO піна
 *
 * @param pin Вказівник на дескриптор піна
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_GPIO_DeInitPin(const HWD_GPIO_Pin_t* pin);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_GPIO_H */
