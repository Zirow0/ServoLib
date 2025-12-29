/**
 * @file board_config.h
 * @brief Конфігурація для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить ПЛАТФОРМО-СПЕЦИФІЧНІ параметри для емуляції на ПК.
 * Загальні параметри проекту налаштовуються в config_user.h.
 *
 * ПРИМІТКА: Для UDP параметрів та емуляційних налаштувань
 * використовуйте тільки цей файл. Параметри мотора, PID, сенсорів
 * налаштовуйте в config_user.h.
 */

#ifndef SERVOCORE_BOARD_EMULATION_CONFIG_H
#define SERVOCORE_BOARD_EMULATION_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/core.h"

/* UDP параметри для емуляції */
#define UDP_SERVER_IP           "127.0.0.1"        /**< IP адреса сервера моделі */
#define UDP_SERVER_PORT         8888               /**< Порт сервера моделі */
#define UDP_CLIENT_PORT         8889               /**< Порт клієнта емулятора */
#define UDP_TIMEOUT_MS          100                /**< Таймаут UDP (мс) */

/* Емуляційні параметри */
#define EMULATION_UPDATE_FREQ   1000.0f            /**< Частота оновлення емуляції (Гц) */
#define EMULATION_TIME_STEP_MS  (1000.0f / EMULATION_UPDATE_FREQ)  /**< Крок часу в мс */

/* Вимкнути реальні апаратні залежності */
#define USE_REAL_HARDWARE       0                  /**< Вимкнути реальне апаратне забезпечення */

/* Вибір драйверів для емуляції */
#define USE_MOTOR_PWM_UDP       1                  /**< Використовувати UDP драйвер для PWM двигуна */
#define USE_BRAKE_UDP           1                  /**< Використовувати UDP драйвер для гальм */

/* Вимкнути реальні драйвери */
#undef USE_MOTOR_PWM                               /**< Не використовувати реальний PWM драйвер */
#undef USE_BRAKE                                   /**< Не використовувати реальний brake драйвер */

/* Таймерні параметри */
#define SYSTEM_TICK_FREQ_HZ     1000               /**< Частота системного таймера (Гц) */

/* Максимальні значення для емуляції */
#define MAX_MOTOR_POWER         100.0f             /**< Максимальна потужність двигуна (%) */
#define MIN_MOTOR_POWER         -100.0f            /**< Мінімальна потужність двигуна (%) */
#define MAX_POSITION_DEGREES    360.0f             /**< Максимальна позиція (градуси) */
#define MIN_POSITION_DEGREES    0.0f               /**< Мінімальна позиція (градуси) */

/* Параметри гальм */
#define BRAKE_DEFAULT_DELAY_MS  100                /**< Затримка відпускання гальм (мс) */
#define BRAKE_TIMEOUT_MS        3000               /**< Таймаут блокування гальм (мс) */

/* Параметри сенсора */
#define SENSOR_DEFAULT_TIMEOUT  50                 /**< Таймаут сенсора (мс) */

#ifdef __cplusplus
}
#endif

/* Function declarations */
Servo_Status_t Board_Init(void);
Servo_Status_t Board_DeInit(void);

#endif /* SERVOCORE_BOARD_EMULATION_CONFIG_H */