/**
 * @file err.c
 * @brief Реалізація обробки помилок
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/ctrl/err.h"
#include "../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/

/**
 * @brief Отримання опису помилки за кодом
 *
 * @param error Код помилки
 * @return const char* Текстовий опис
 */
static const char* GetErrorDescription(Servo_Error_t error)
{
    switch (error) {
        case ERR_NONE:              return "No error";
        case ERR_MOTOR_OVERCURRENT: return "Motor overcurrent";
        case ERR_MOTOR_OVERHEAT:    return "Motor overheat";
        case ERR_MOTOR_STALL:       return "Motor stall";
        case ERR_SENSOR_LOST:       return "Sensor communication lost";
        case ERR_SENSOR_INVALID:    return "Invalid sensor data";
        case ERR_POSITION_LIMIT:    return "Position limit exceeded";
        case ERR_VELOCITY_LIMIT:    return "Velocity limit exceeded";
        case ERR_WATCHDOG:          return "Watchdog timeout";
        case ERR_INIT_FAILED:       return "Initialization failed";
        default:                    return "Unknown error";
    }
}

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Error_Init(Error_Manager_t* manager)
{
    if (manager == NULL) {
        return SERVO_INVALID;
    }

    // Очищення структури
    memset(manager, 0, sizeof(Error_Manager_t));

    manager->is_initialized = true;

    return SERVO_OK;
}

Servo_Status_t Error_Log(Error_Manager_t* manager,
                         Servo_Error_t error,
                         Error_Severity_t severity,
                         const char* message)
{
    if (manager == NULL || !manager->is_initialized) {
        return SERVO_INVALID;
    }

    // Перевірка чи ця помилка вже є в останньому записі
    if (manager->log_count > 0) {
        uint32_t last_idx = (manager->log_tail == 0) ?
                            (ERROR_LOG_BUFFER_SIZE - 1) :
                            (manager->log_tail - 1);

        if (manager->log[last_idx].code == error) {
            // Збільшуємо лічильник повторень
            manager->log[last_idx].count++;
            manager->log[last_idx].timestamp = HWD_Timer_GetMillis();
            return SERVO_OK;
        }
    }

    // Додавання нового запису
    Error_Record_t* record = &manager->log[manager->log_tail];

    record->code = error;
    record->severity = severity;
    record->timestamp = HWD_Timer_GetMillis();
    record->count = 1;

    // Копіювання повідомлення
    if (message != NULL) {
        strncpy(record->message, message, sizeof(record->message) - 1);
        record->message[sizeof(record->message) - 1] = '\0';
    } else {
        // Використовуємо стандартний опис
        const char* desc = Error_GetDescription(error);
        strncpy(record->message, desc, sizeof(record->message) - 1);
        record->message[sizeof(record->message) - 1] = '\0';
    }

    // Оновлення індексів
    manager->log_tail = (manager->log_tail + 1) % ERROR_LOG_BUFFER_SIZE;

    if (manager->log_count < ERROR_LOG_BUFFER_SIZE) {
        manager->log_count++;
    } else {
        // Буфер повний, переміщуємо head
        manager->log_head = (manager->log_head + 1) % ERROR_LOG_BUFFER_SIZE;
    }

    // Оновлення статистики
    manager->last_error = error;
    manager->last_severity = severity;

    if (severity >= ERR_SEVERITY_ERROR) {
        manager->total_errors++;
    } else if (severity == ERR_SEVERITY_WARNING) {
        manager->total_warnings++;
    }

    // Виклик callback для критичних помилок
    if (severity == ERR_SEVERITY_CRITICAL && manager->callback != NULL) {
        manager->callback(error, severity);
    }

    return SERVO_OK;
}

Servo_Status_t Error_GetLast(Error_Manager_t* manager,
                             Servo_Error_t* error,
                             Error_Severity_t* severity)
{
    if (manager == NULL || !manager->is_initialized) {
        return SERVO_INVALID;
    }

    if (error != NULL) {
        *error = manager->last_error;
    }

    if (severity != NULL) {
        *severity = manager->last_severity;
    }

    return SERVO_OK;
}

Servo_Status_t Error_ClearLog(Error_Manager_t* manager)
{
    if (manager == NULL || !manager->is_initialized) {
        return SERVO_INVALID;
    }

    manager->log_head = 0;
    manager->log_tail = 0;
    manager->log_count = 0;
    manager->last_error = ERR_NONE;
    manager->last_severity = ERR_SEVERITY_INFO;

    return SERVO_OK;
}

uint32_t Error_GetCount(const Error_Manager_t* manager)
{
    if (manager == NULL || !manager->is_initialized) {
        return 0;
    }

    return manager->log_count;
}

Servo_Status_t Error_GetRecord(const Error_Manager_t* manager,
                               uint32_t index,
                               Error_Record_t* record)
{
    if (manager == NULL || !manager->is_initialized || record == NULL) {
        return SERVO_INVALID;
    }

    if (index >= manager->log_count) {
        return SERVO_INVALID;
    }

    // Індекс 0 = найновіший запис
    uint32_t actual_idx = (manager->log_tail - 1 - index + ERROR_LOG_BUFFER_SIZE) % ERROR_LOG_BUFFER_SIZE;

    *record = manager->log[actual_idx];

    return SERVO_OK;
}

Servo_Status_t Error_SetCallback(Error_Manager_t* manager, Error_Callback_t callback)
{
    if (manager == NULL || !manager->is_initialized) {
        return SERVO_INVALID;
    }

    manager->callback = callback;

    return SERVO_OK;
}

bool Error_HasCritical(const Error_Manager_t* manager)
{
    if (manager == NULL || !manager->is_initialized) {
        return false;
    }

    // Перевірка останньої помилки
    if (manager->last_severity == ERR_SEVERITY_CRITICAL) {
        return true;
    }

    // Перевірка всього логу
    for (uint32_t i = 0; i < manager->log_count; i++) {
        uint32_t idx = (manager->log_head + i) % ERROR_LOG_BUFFER_SIZE;
        if (manager->log[idx].severity == ERR_SEVERITY_CRITICAL) {
            return true;
        }
    }

    return false;
}

const char* Error_GetDescription(Servo_Error_t error)
{
    return GetErrorDescription(error);
}

Servo_Status_t Error_GetStats(const Error_Manager_t* manager,
                              uint32_t* total_errors,
                              uint32_t* total_warnings)
{
    if (manager == NULL || !manager->is_initialized) {
        return SERVO_INVALID;
    }

    if (total_errors != NULL) {
        *total_errors = manager->total_errors;
    }

    if (total_warnings != NULL) {
        *total_warnings = manager->total_warnings;
    }

    return SERVO_OK;
}
