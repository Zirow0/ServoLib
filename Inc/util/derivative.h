/**
 * @file derivative.h
 * @brief Обчислення похідної (швидкості) для датчиків положення
 * @author ServoCore Team
 * @date 2025
 *
 * Модуль для обчислення velocity з позиції та часу.
 * Підтримує нормалізацію переходу через 0/360 градусів.
 */

#ifndef SERVOCORE_UTIL_DERIVATIVE_H
#define SERVOCORE_UTIL_DERIVATIVE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Нормалізація різниці кутів з врахуванням переходу через 0/360
 *
 * Перетворює різницю кутів у діапазон [-180, 180] градусів.
 * Приклад: delta = 350° - 10° = 340° → нормалізоване = -20°
 *
 * @param delta_deg Різниця кутів (градуси)
 * @return float Нормалізована різниця [-180, 180]
 */
float Derivative_NormalizeAngleDelta(float delta_deg);

/**
 * @brief Обчислення швидкості з позиції та часу
 *
 * Обчислює швидкість (град/с) з двох послідовних вимірювань позиції.
 * Автоматично враховує перехід через 0/360 градусів.
 *
 * @param current_pos Поточна позиція (градуси)
 * @param last_pos Попередня позиція (градуси)
 * @param current_time_us Поточний час (мікросекунди)
 * @param last_time_us Попередній час (мікросекунди)
 * @return float Швидкість (град/с), 0.0f якщо dt = 0
 */
float Derivative_CalculateVelocity(float current_pos, float last_pos,
                                   uint32_t current_time_us, uint32_t last_time_us);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_UTIL_DERIVATIVE_H */
