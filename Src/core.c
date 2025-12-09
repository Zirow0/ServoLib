/**
 * @file core.c
 * @brief Реалізація основних функцій бібліотеки ServoCore
 * @author ServoCore Team
 * @date 2025
 *
 * Реалізація допоміжних функцій для роботи з основними типами даних
 */

/* Includes ------------------------------------------------------------------*/
#include "../Inc/core.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/** @brief Таблиця текстових описів статусів */
static const char* servo_status_strings[] = {
    "OK",
    "ERROR",
    "BUSY",
    "TIMEOUT",
    "INVALID",
    "NOT_INIT"
};

/** @brief Таблиця текстових описів режимів */
static const char* servo_mode_strings[] = {
    "IDLE",
    "POSITION",
    "VELOCITY",
    "TORQUE",
    "CALIBRATION"
};

/** @brief Таблиця текстових описів станів */
static const char* servo_state_strings[] = {
    "UNINIT",
    "READY",
    "RUNNING",
    "ERROR",
    "EMERGENCY"
};

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Отримання текстового опису статусу
 *
 * @param status Код статусу
 * @return const char* Текстовий опис
 */
const char* Servo_GetStatusString(Servo_Status_t status)
{
    if (status < sizeof(servo_status_strings) / sizeof(servo_status_strings[0])) {
        return servo_status_strings[status];
    }
    return "UNKNOWN";
}

/**
 * @brief Отримання текстового опису режиму
 *
 * @param mode Режим роботи
 * @return const char* Текстовий опис
 */
const char* Servo_GetModeString(Servo_Mode_t mode)
{
    if (mode == SERVO_MODE_ERROR) {
        return "ERROR";
    }
    if (mode < sizeof(servo_mode_strings) / sizeof(servo_mode_strings[0])) {
        return servo_mode_strings[mode];
    }
    return "UNKNOWN";
}

/**
 * @brief Отримання текстового опису стану
 *
 * @param state Стан сервоприводу
 * @return const char* Текстовий опис
 */
const char* Servo_GetStateString(Servo_State_t state)
{
    if (state < sizeof(servo_state_strings) / sizeof(servo_state_strings[0])) {
        return servo_state_strings[state];
    }
    return "UNKNOWN";
}

/**
 * @brief Ініціалізація структури конфігурації осі за замовчуванням
 *
 * @param config Вказівник на структуру конфігурації
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Axis_InitDefaultConfig(Axis_Config_t* config)
{
    if (config == NULL) {
        return SERVO_INVALID;
    }

    memset(config, 0, sizeof(Axis_Config_t));

    // Встановлення значень за замовчуванням
    config->motor_type = MOTOR_TYPE_DC_PWM;
    config->sensor_type = SENSOR_TYPE_ENCODER_MAG;
    config->max_velocity = 100.0f;      // град/с
    config->max_acceleration = 500.0f;  // град/с²
    config->max_current = 2.0f;         // А
    config->position_min = -180.0f;     // град
    config->position_max = 180.0f;      // град
    config->enable_limits = true;

    return SERVO_OK;
}

/**
 * @brief Ініціалізація структури стану осі
 *
 * @param state Вказівник на структуру стану
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Axis_InitState(Axis_State_t* state)
{
    if (state == NULL) {
        return SERVO_INVALID;
    }

    memset(state, 0, sizeof(Axis_State_t));

    state->state = SERVO_STATE_UNINIT;
    state->mode = SERVO_MODE_IDLE;

    return SERVO_OK;
}

/**
 * @brief Перевірка чи статус є помилкою
 *
 * @param status Код статусу
 * @return bool true якщо це помилка
 */
bool Servo_IsError(Servo_Status_t status)
{
    return (status != SERVO_OK);
}

/**
 * @brief Перевірка чи режим потребує датчика
 *
 * @param mode Режим роботи
 * @return bool true якщо потрібен датчик
 */
bool Servo_ModeRequiresSensor(Servo_Mode_t mode)
{
    return (mode == SERVO_MODE_POSITION ||
            mode == SERVO_MODE_VELOCITY ||
            mode == SERVO_MODE_CALIBRATION);
}

/**
 * @brief Валідація конфігурації осі
 *
 * @param config Вказівник на структуру конфігурації
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Axis_ValidateConfig(const Axis_Config_t* config)
{
    if (config == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка меж швидкості
    if (config->max_velocity <= 0.0f) {
        return SERVO_INVALID;
    }

    // Перевірка меж прискорення
    if (config->max_acceleration <= 0.0f) {
        return SERVO_INVALID;
    }

    // Перевірка струму
    if (config->max_current <= 0.0f) {
        return SERVO_INVALID;
    }

    // Перевірка меж положення
    if (config->enable_limits) {
        if (config->position_min >= config->position_max) {
            return SERVO_INVALID;
        }
    }

    return SERVO_OK;
}
