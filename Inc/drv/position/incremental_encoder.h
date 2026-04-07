/**
 * @file incremental_encoder.h
 * @brief Драйвер інкрементального квадратурного енкодера
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізує Hardware Callbacks Pattern для Position_Sensor_Interface_t.
 * Апаратний підрахунок імпульсів через HWD_Timer (режим квадратурного
 * енкодера x4), вся логіка обробки (velocity, multi-turn, prediction)
 * залишається в position.c.
 */

#ifndef SERVOCORE_DRV_INCREMENTAL_ENCODER_H
#define SERVOCORE_DRV_INCREMENTAL_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "position.h"
#include "../../hwd/hwd_timer.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Конфігурація драйвера інкрементального енкодера
 */
typedef struct {
    uint32_t cpr;                    /**< Counts Per Revolution після x4 квадратури */
    HWD_Encoder_Config_t hw;         /**< Конфігурація апаратного таймера */
} Incremental_Encoder_Config_t;

/**
 * @brief Структура драйвера інкрементального енкодера
 *
 * Перше поле — Position_Sensor_Interface_t (обов'язково!).
 * Вся логіка (velocity, multi-turn, prediction) обробляється в position.c.
 */
typedef struct {
    Position_Sensor_Interface_t interface;  /**< Універсальний інтерфейс (ПЕРШИЙ!) */
    Incremental_Encoder_Config_t config;    /**< Конфігурація */
    HWD_Encoder_Handle_t handle;            /**< Дескриптор таймера-енкодера */
} Incremental_Encoder_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера інкрементального енкодера
 *
 * Зберігає конфігурацію та прив'язує hardware callbacks до інтерфейсу.
 * Після створення ініціалізуйте через Position_Sensor_Init(&driver->interface, params).
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t* driver,
                                           const Incremental_Encoder_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_INCREMENTAL_ENCODER_H */
