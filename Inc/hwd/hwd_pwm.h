/**
 * @file hwd_pwm.h
 * @brief Апаратна абстракція PWM сигналів
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл визначає незалежний від платформи інтерфейс для роботи з PWM.
 * Конкретна реалізація надається в Board/STM32F411/hwd_pwm.c
 */

#ifndef SERVOCORE_HWD_PWM_H
#define SERVOCORE_HWD_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація PWM каналу
 */
typedef struct {
    uint32_t frequency;   /**< Частота PWM (Hz) */
    uint32_t resolution;  /**< Роздільна здатність (кількість кроків) */
    void*    hw_handle;   /**< Базова адреса таймера (платформо-специфічна) */
    uint32_t hw_channel;  /**< Апаратний канал таймера (enum tim_oc_id) */
} HWD_PWM_Config_t;

/**
 * @brief Структура PWM дескриптора
 */
typedef struct {
    HWD_PWM_Config_t config;  /**< Конфігурація */
} HWD_PWM_Handle_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація PWM каналу
 *
 * @param handle Вказівник на дескриптор PWM
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_Init(HWD_PWM_Handle_t* handle, const HWD_PWM_Config_t* config);

/**
 * @brief Запуск генерації PWM
 *
 * @param handle Вказівник на дескриптор PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle);

/**
 * @brief Встановлення коефіцієнту заповнення в процентах
 *
 * @param handle Вказівник на дескриптор PWM
 * @param percent Відсоток (0.0 - 100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float percent);


#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_PWM_H */
