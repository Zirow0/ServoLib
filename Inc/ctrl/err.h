/**
 * @file err.h
 * @brief Обробка помилок та кодів
 * @author ServoCore Team
 * @date 2025
 *
 * Централізована система обробки та логування помилок
 */

#ifndef SERVOCORE_CTRL_ERR_H
#define SERVOCORE_CTRL_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "../config.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Рівень критичності помилки
 */
typedef enum {
    ERR_SEVERITY_INFO    = 0,  /**< Інформаційне повідомлення */
    ERR_SEVERITY_WARNING = 1,  /**< Попередження */
    ERR_SEVERITY_ERROR   = 2,  /**< Помилка */
    ERR_SEVERITY_CRITICAL = 3  /**< Критична помилка */
} Error_Severity_t;

/**
 * @brief Запис помилки в логу
 */
typedef struct {
    Servo_Error_t code;        /**< Код помилки */
    Error_Severity_t severity; /**< Рівень критичності */
    uint32_t timestamp;        /**< Мітка часу (мс) */
    uint32_t count;            /**< Кількість повторень */
    char message[64];          /**< Текстове повідомлення */
} Error_Record_t;

/**
 * @brief Callback функція для критичних помилок
 */
typedef void (*Error_Callback_t)(Servo_Error_t error, Error_Severity_t severity);

/**
 * @brief Структура менеджера помилок
 */
typedef struct {
    Error_Record_t log[ERROR_LOG_BUFFER_SIZE]; /**< Буфер логу помилок */
    uint32_t log_head;                         /**< Індекс початку */
    uint32_t log_tail;                         /**< Індекс кінця */
    uint32_t log_count;                        /**< Кількість записів */

    Servo_Error_t last_error;                  /**< Останній код помилки */
    Error_Severity_t last_severity;            /**< Остання критичність */

    Error_Callback_t callback;                 /**< Callback для критичних помилок */

    uint32_t total_errors;                     /**< Загальна кількість помилок */
    uint32_t total_warnings;                   /**< Загальна кількість попереджень */

    bool is_initialized;                       /**< Прапорець ініціалізації */
} Error_Manager_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація менеджера помилок
 *
 * @param manager Вказівник на менеджер помилок
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_Init(Error_Manager_t* manager);

/**
 * @brief Реєстрація помилки
 *
 * @param manager Вказівник на менеджер помилок
 * @param error Код помилки
 * @param severity Рівень критичності
 * @param message Текстове повідомлення (може бути NULL)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_Log(Error_Manager_t* manager,
                         Servo_Error_t error,
                         Error_Severity_t severity,
                         const char* message);

/**
 * @brief Отримання останньої помилки
 *
 * @param manager Вказівник на менеджер помилок
 * @param error Вказівник для збереження коду помилки
 * @param severity Вказівник для збереження критичності
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_GetLast(Error_Manager_t* manager,
                             Servo_Error_t* error,
                             Error_Severity_t* severity);

/**
 * @brief Очищення логу помилок
 *
 * @param manager Вказівник на менеджер помилок
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_ClearLog(Error_Manager_t* manager);

/**
 * @brief Отримання кількості помилок в логу
 *
 * @param manager Вказівник на менеджер помилок
 * @return uint32_t Кількість записів
 */
uint32_t Error_GetCount(const Error_Manager_t* manager);

/**
 * @brief Отримання запису з логу
 *
 * @param manager Вказівник на менеджер помилок
 * @param index Індекс запису (0 = найновіший)
 * @param record Вказівник для збереження запису
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_GetRecord(const Error_Manager_t* manager,
                               uint32_t index,
                               Error_Record_t* record);

/**
 * @brief Встановлення callback функції
 *
 * @param manager Вказівник на менеджер помилок
 * @param callback Функція callback (NULL для відключення)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_SetCallback(Error_Manager_t* manager, Error_Callback_t callback);

/**
 * @brief Перевірка наявності критичних помилок
 *
 * @param manager Вказівник на менеджер помилок
 * @return bool true якщо є критичні помилки
 */
bool Error_HasCritical(const Error_Manager_t* manager);

/**
 * @brief Отримання текстового опису помилки
 *
 * @param error Код помилки
 * @return const char* Текстовий опис
 */
const char* Error_GetDescription(Servo_Error_t error);

/**
 * @brief Отримання статистики помилок
 *
 * @param manager Вказівник на менеджер помилок
 * @param total_errors Вказівник для загальної кількості помилок
 * @param total_warnings Вказівник для загальної кількості попереджень
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Error_GetStats(const Error_Manager_t* manager,
                              uint32_t* total_errors,
                              uint32_t* total_warnings);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_ERR_H */
