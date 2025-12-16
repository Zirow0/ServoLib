/**
 * @file config_user.h
 * @brief Користувацька конфігурація для емулятора ServoLib
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить налаштування, специфічні для емулятора.
 */

#ifndef SERVOCORE_CONFIG_USER_H
#define SERVOCORE_CONFIG_USER_H

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Конфігурація для емулятора
 * ===========================================================================*/

/**
 * @brief Частота оновлення контуру керування (Hz)
 * @note Для емулятора зменшено до 200 Hz для зменшення навантаження
 */
#define CONTROL_LOOP_FREQ 200U  /* 20 Hz для емулятора */

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CONFIG_USER_H */