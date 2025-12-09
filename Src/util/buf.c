/**
 * @file buf.c
 * @brief Реалізація кільцевого буфера
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/util/buf.h"
#include <string.h>

/* Exported functions --------------------------------------------------------*/

Servo_Status_t RingBuf_Init(RingBuffer_t* rb,
                            uint8_t* buffer,
                            uint32_t capacity,
                            uint32_t element_size)
{
    if (rb == NULL || buffer == NULL || capacity == 0 || element_size == 0) {
        return SERVO_INVALID;
    }

    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->element_size = element_size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->is_initialized = true;

    // Очищення буфера
    memset(buffer, 0, capacity * element_size);

    return SERVO_OK;
}

Servo_Status_t RingBuf_Put(RingBuffer_t* rb, const void* data)
{
    if (rb == NULL || !rb->is_initialized || data == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка чи буфер повний
    if (rb->count >= rb->capacity) {
        return SERVO_BUSY;
    }

    // Копіювання даних в буфер
    uint32_t offset = rb->head * rb->element_size;
    memcpy(&rb->buffer[offset], data, rb->element_size);

    // Оновлення індексів
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count++;

    return SERVO_OK;
}

Servo_Status_t RingBuf_Get(RingBuffer_t* rb, void* data)
{
    if (rb == NULL || !rb->is_initialized || data == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка чи буфер порожній
    if (rb->count == 0) {
        return SERVO_ERROR;
    }

    // Копіювання даних з буфера
    uint32_t offset = rb->tail * rb->element_size;
    memcpy(data, &rb->buffer[offset], rb->element_size);

    // Оновлення індексів
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;

    return SERVO_OK;
}

Servo_Status_t RingBuf_Peek(const RingBuffer_t* rb, void* data, uint32_t index)
{
    if (rb == NULL || !rb->is_initialized || data == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка чи індекс в межах
    if (index >= rb->count) {
        return SERVO_INVALID;
    }

    // Обчислення реального індексу
    uint32_t actual_index = (rb->tail + index) % rb->capacity;
    uint32_t offset = actual_index * rb->element_size;

    // Копіювання даних
    memcpy(data, &rb->buffer[offset], rb->element_size);

    return SERVO_OK;
}

bool RingBuf_IsEmpty(const RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized) {
        return true;
    }

    return (rb->count == 0);
}

bool RingBuf_IsFull(const RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized) {
        return false;
    }

    return (rb->count >= rb->capacity);
}

uint32_t RingBuf_GetCount(const RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized) {
        return 0;
    }

    return rb->count;
}

uint32_t RingBuf_GetFree(const RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized) {
        return 0;
    }

    return rb->capacity - rb->count;
}

Servo_Status_t RingBuf_Clear(RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized) {
        return SERVO_INVALID;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    // Опціонально: очищення буфера
    memset(rb->buffer, 0, rb->capacity * rb->element_size);

    return SERVO_OK;
}

float RingBuf_GetFillPercentage(const RingBuffer_t* rb)
{
    if (rb == NULL || !rb->is_initialized || rb->capacity == 0) {
        return 0.0f;
    }

    return ((float)rb->count / (float)rb->capacity) * 100.0f;
}
