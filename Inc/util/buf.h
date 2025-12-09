/**
 * @file buf.h
 * @brief Кільцевий буфер (Ring Buffer / Circular Buffer)
 * @author ServoCore Team
 * @date 2025
 *
 * Ефективна структура даних для FIFO черги з фіксованим розміром
 */

#ifndef SERVOCORE_UTIL_BUF_H
#define SERVOCORE_UTIL_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Структура кільцевого буфера
 */
typedef struct {
    uint8_t* buffer;         /**< Вказівник на буфер */
    uint32_t capacity;       /**< Ємність буфера */
    uint32_t head;           /**< Індекс голови (запис) */
    uint32_t tail;           /**< Індекс хвоста (читання) */
    uint32_t count;          /**< Кількість елементів */
    uint32_t element_size;   /**< Розмір одного елемента (байт) */
    bool is_initialized;     /**< Прапорець ініціалізації */
} RingBuffer_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація кільцевого буфера
 *
 * @param rb Вказівник на структуру буфера
 * @param buffer Вказівник на масив буфера
 * @param capacity Ємність (кількість елементів)
 * @param element_size Розмір одного елемента (байт)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t RingBuf_Init(RingBuffer_t* rb,
                            uint8_t* buffer,
                            uint32_t capacity,
                            uint32_t element_size);

/**
 * @brief Додавання елемента в буфер
 *
 * @param rb Вказівник на структуру буфера
 * @param data Вказівник на дані для додавання
 * @return Servo_Status_t SERVO_OK або SERVO_BUSY якщо буфер повний
 */
Servo_Status_t RingBuf_Put(RingBuffer_t* rb, const void* data);

/**
 * @brief Отримання елемента з буфера
 *
 * @param rb Вказівник на структуру буфера
 * @param data Вказівник для збереження даних
 * @return Servo_Status_t SERVO_OK або SERVO_ERROR якщо буфер порожній
 */
Servo_Status_t RingBuf_Get(RingBuffer_t* rb, void* data);

/**
 * @brief Перегляд елемента без видалення
 *
 * @param rb Вказівник на структуру буфера
 * @param data Вказівник для збереження даних
 * @param index Індекс від хвоста (0 = перший елемент)
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t RingBuf_Peek(const RingBuffer_t* rb, void* data, uint32_t index);

/**
 * @brief Перевірка чи буфер порожній
 *
 * @param rb Вказівник на структуру буфера
 * @return bool true якщо порожній
 */
bool RingBuf_IsEmpty(const RingBuffer_t* rb);

/**
 * @brief Перевірка чи буфер повний
 *
 * @param rb Вказівник на структуру буфера
 * @return bool true якщо повний
 */
bool RingBuf_IsFull(const RingBuffer_t* rb);

/**
 * @brief Отримання кількості елементів в буфері
 *
 * @param rb Вказівник на структуру буфера
 * @return uint32_t Кількість елементів
 */
uint32_t RingBuf_GetCount(const RingBuffer_t* rb);

/**
 * @brief Отримання вільного місця в буфері
 *
 * @param rb Вказівник на структуру буфера
 * @return uint32_t Кількість вільних місць
 */
uint32_t RingBuf_GetFree(const RingBuffer_t* rb);

/**
 * @brief Очищення буфера
 *
 * @param rb Вказівник на структуру буфера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t RingBuf_Clear(RingBuffer_t* rb);

/**
 * @brief Отримання відсотка заповненості
 *
 * @param rb Вказівник на структуру буфера
 * @return float Відсоток заповненості (0.0 - 100.0)
 */
float RingBuf_GetFillPercentage(const RingBuffer_t* rb);

/* Макроси для типізованих буферів -------------------------------------------*/

/**
 * @brief Макрос для створення типізованого буфера
 *
 * Приклад:
 * RINGBUF_DEFINE(float, float_buffer, 32);
 * це створить:
 * - RingBuffer_t float_buffer_rb;
 * - float float_buffer_data[32];
 */
#define RINGBUF_DEFINE(type, name, size) \
    static type name##_data[size]; \
    static RingBuffer_t name##_rb

/**
 * @brief Макрос для ініціалізації типізованого буфера
 */
#define RINGBUF_INIT(name, size) \
    RingBuf_Init(&name##_rb, (uint8_t*)name##_data, size, sizeof(name##_data[0]))

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_UTIL_BUF_H */
