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
#include "../config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Канали PWM
 */
typedef enum {
    HWD_PWM_CHANNEL_1 = 0,
    HWD_PWM_CHANNEL_2 = 1,
    HWD_PWM_CHANNEL_3 = 2,
    HWD_PWM_CHANNEL_4 = 3
} HWD_PWM_Channel_t;

/**
 * @brief Режими PWM
 */
typedef enum {
    HWD_PWM_MODE_FORWARD  = 0,  /**< Прямий хід (один канал активний) */
    HWD_PWM_MODE_REVERSE  = 1,  /**< Зворотний хід (інший канал активний) */
    HWD_PWM_MODE_BRAKE    = 2   /**< Гальмування (обидва канали активні) */
} HWD_PWM_Mode_t;

/**
 * @brief Конфігурація PWM каналу
 */
typedef struct {
    uint32_t frequency;          /**< Частота PWM (Hz) */
    uint32_t resolution;         /**< Роздільна здатність (кількість кроків) */
    HWD_PWM_Channel_t channel;   /**< Номер каналу */
    void* hw_handle;             /**< Вказівник на апаратний дескриптор (TIM_HandleTypeDef) */
    uint32_t hw_channel;         /**< Апаратний канал таймера */
} HWD_PWM_Config_t;

/**
 * @brief Структура PWM дескриптора
 */
typedef struct {
    HWD_PWM_Config_t config;     /**< Конфігурація */
    uint32_t current_duty;       /**< Поточний duty cycle (0-resolution) */
    HWD_PWM_Mode_t mode;         /**< Поточний режим */
    bool is_running;             /**< Прапорець активності */
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
 * @brief Деініціалізація PWM каналу
 *
 * @param handle Вказівник на дескриптор PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_DeInit(HWD_PWM_Handle_t* handle);

/**
 * @brief Запуск генерації PWM
 *
 * @param handle Вказівник на дескриптор PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_Start(HWD_PWM_Handle_t* handle);

/**
 * @brief Зупинка генерації PWM
 *
 * @param handle Вказівник на дескриптор PWM
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_Stop(HWD_PWM_Handle_t* handle);

/**
 * @brief Встановлення коефіцієнту заповнення (duty cycle)
 *
 * @param handle Вказівник на дескриптор PWM
 * @param duty Значення duty cycle (0 - resolution)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_SetDuty(HWD_PWM_Handle_t* handle, uint32_t duty);

/**
 * @brief Встановлення коефіцієнту заповнення в процентах
 *
 * @param handle Вказівник на дескриптор PWM
 * @param percent Відсоток (0.0 - 100.0)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float percent);

/**
 * @brief Отримання поточного duty cycle
 *
 * @param handle Вказівник на дескриптор PWM
 * @return uint32_t Поточне значення duty cycle
 */
uint32_t HWD_PWM_GetDuty(const HWD_PWM_Handle_t* handle);

/**
 * @brief Встановлення частоти PWM
 *
 * @param handle Вказівник на дескриптор PWM
 * @param frequency Нова частота (Hz)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_SetFrequency(HWD_PWM_Handle_t* handle, uint32_t frequency);

/**
 * @brief Встановлення режиму PWM
 *
 * @param handle Вказівник на дескриптор PWM
 * @param mode Режим роботи
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t HWD_PWM_SetMode(HWD_PWM_Handle_t* handle, HWD_PWM_Mode_t mode);

/**
 * @brief Перевірка стану PWM каналу
 *
 * @param handle Вказівник на дескриптор PWM
 * @return bool true якщо працює, false якщо зупинено
 */
bool HWD_PWM_IsRunning(const HWD_PWM_Handle_t* handle);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_HWD_PWM_H */
