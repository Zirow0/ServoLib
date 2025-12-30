/**
 * @file prediction.h
 * @brief Передбачення позиції (екстраполяція) для датчиків положення
 * @author ServoCore Team
 * @date 2025
 *
 * Модуль для лінійної екстраполяції позиції між updates.
 * Використовується для отримання актуальної позиції між вимірюваннями.
 */

#ifndef SERVOCORE_UTIL_PREDICTION_H
#define SERVOCORE_UTIL_PREDICTION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Лінійна екстраполяція позиції
 *
 * Обчислює позицію через dt_us, використовуючи поточну позицію та швидкість.
 * Формула: predicted_pos = position + velocity * dt
 *
 * @param position_deg Поточна позиція (градуси)
 * @param velocity_deg_s Швидкість (град/с)
 * @param dt_us Інтервал екстраполяції (мікросекунди)
 * @return float Передбачена позиція (градуси, 0-360)
 */
float Prediction_LinearExtrapolate(float position_deg, float velocity_deg_s, uint32_t dt_us);

/**
 * @brief Отримання передбаченої позиції на поточний момент
 *
 * Обчислює позицію на current_time_us, екстраполюючи від last_time_us.
 *
 * @param last_position_deg Остання виміряна позиція (градуси)
 * @param velocity_deg_s Швидкість (град/с)
 * @param last_time_us Час останнього вимірювання (мкс)
 * @param current_time_us Поточний час (мкс)
 * @return float Передбачена позиція (градуси, 0-360)
 */
float Prediction_GetCurrentPosition(float last_position_deg, float velocity_deg_s,
                                   uint32_t last_time_us, uint32_t current_time_us);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_UTIL_PREDICTION_H */
