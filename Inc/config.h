/**
 * @file config.h
 * @brief Головний файл конфігурації бібліотеки ServoCore
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл є "диспетчером", який об'єднує всі шари конфігурації:
 * 1. config_lib.h - універсальні математичні константи
 * 2. config_user.h - конфігурація проекту (якщо існує)
 * 3. config_defaults.h - дефолтні значення бібліотеки
 *
 * ПОРЯДОК ВКЛЮЧЕННЯ КРИТИЧНИЙ:
 * - config_user.h включається ПЕРЕД config_defaults.h
 * - Завдяки #ifndef в config_defaults.h параметри з config_user.h мають пріоритет
 *
 * ВИКОРИСТАННЯ:
 * - Для використання дефолтних значень: просто #include "config.h"
 * - Для власної конфігурації: створіть config_user.h у вашому проекті
 *
 * ПРИКЛАД СТРУКТУРИ ПРОЕКТУ:
 * YourProject/
 * ├── Inc/
 * │   └── config_user.h       # Ваша конфігурація
 * └── Libs/
 *     └── ServoLib/
 *         └── Inc/
 *             ├── config.h          # Цей файл
 *             ├── config_lib.h
 *             └── config_defaults.h
 */

#ifndef SERVOCORE_CONFIG_H
#define SERVOCORE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "core.h"  /* Базові типи та константи */

/* =============================================================================
 * ШАР 1: Універсальні константи бібліотеки
 * ===========================================================================*/

#include "config_lib.h"  /* Математичні константи, конверсії */

/* =============================================================================
 * ШАР 2: Конфігурація проекту (ПЕРЕД defaults для можливості override!)
 * ===========================================================================*/

/*
 * Спроба знайти config_user.h в директоріях проекту.
 * Якщо файл не знайдено, використовуються дефолтні значення.
 */

#if defined(__has_include)
    /* C11/C++17 та новіші: використовуємо __has_include() */
    #if __has_include("config_user.h")
        #define CONFIG_USER_LOADED
        #include "config_user.h"
    #elif __has_include("../../Inc/config_user.h")
        /* Альтернативний шлях для STM32CubeIDE (з директорії ServoLib/Inc/) */
        #define CONFIG_USER_LOADED
        #include "../../Inc/config_user.h"
    #elif __has_include("../../../Inc/config_user.h")
        /* Ще один альтернативний шлях (з піддиректорії) */
        #define CONFIG_USER_LOADED
        #include "../../../Inc/config_user.h"
    #else
        /* config_user.h не знайдено - використовуємо дефолти */
        #pragma message("INFO: config_user.h not found - using library default configuration")
    #endif
#else
    /* Для старих компіляторів без __has_include() */
    /* Користувач має додати include path до config_user.h вручну */
    #ifdef CONFIG_USER_PROVIDED
        #include "config_user.h"
        #define CONFIG_USER_LOADED
    #else
        #pragma message("INFO: CONFIG_USER_PROVIDED not defined - using library default configuration")
    #endif
#endif

/* =============================================================================
 * ШАР 3: Дефолтні значення бібліотеки
 * ===========================================================================*/

/*
 * config_defaults.h визначає всі параметри з обгорткою #ifndef.
 * Якщо параметр вже визначено в config_user.h, він НЕ буде перевизначено.
 */

#include "config_defaults.h"

/* =============================================================================
 * Валідація логічної узгодженості конфігурації
 * ===========================================================================*/

/*
 * Перевіряємо логічну узгодженість параметрів (не жорсткі межі).
 * Це допомагає виявити помилки конфігурації на етапі компіляції.
 */

#if MOTOR_OVERCURRENT_LIMIT <= MOTOR_MAX_CURRENT
    #warning "MOTOR_OVERCURRENT_LIMIT should be greater than MOTOR_MAX_CURRENT for proper protection"
#endif

#if MOTOR_MAX_CURRENT > ABSOLUTE_MAX_CURRENT
    #warning "MOTOR_MAX_CURRENT exceeds ABSOLUTE_MAX_CURRENT - verify your hardware specifications"
#endif

#if MOTOR_MAX_TEMPERATURE > ABSOLUTE_MAX_TEMP
    #warning "MOTOR_MAX_TEMPERATURE exceeds ABSOLUTE_MAX_TEMP - verify your hardware specifications"
#endif

#if DEFAULT_MAX_VELOCITY > ABSOLUTE_MAX_VELOCITY
    #warning "DEFAULT_MAX_VELOCITY exceeds ABSOLUTE_MAX_VELOCITY - this may cause issues"
#endif

/* =============================================================================
 * Інформаційні повідомлення компіляції
 * ===========================================================================*/

#ifdef CONFIG_USER_LOADED
    #pragma message("=== ServoLib Configuration ===")
    #pragma message("Using project-specific configuration from config_user.h")
#else
    #pragma message("=== ServoLib Configuration ===")
    #pragma message("Using library default configuration (config_defaults.h)")
    #pragma message("To customize: create config_user.h in your project's Inc/ directory")
#endif

/* Виводимо ключові параметри для зручності */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#pragma message("Control loop frequency: " TOSTRING(CONTROL_LOOP_FREQ) " Hz")
#pragma message("Motor max current: " TOSTRING(MOTOR_MAX_CURRENT) " mA")
#pragma message("Encoder resolution: " TOSTRING(ENCODER_RESOLUTION_BITS) " bits")

/* =============================================================================
 * Підтримка різних платформ
 * ===========================================================================*/

/*
 * Платформо-специфічні includes (за потреби).
 * Рекомендується використовувати board_config.h для платформенної конфігурації.
 */

#ifdef STM32F411xE
    /* STM32F411 специфічні налаштування (якщо потрібно) */
#endif

#ifdef PC_EMULATION
    /* PC Emulation специфічні налаштування (якщо потрібно) */
#endif

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CONFIG_H */
