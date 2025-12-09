/**
 * @file math.h
 * @brief Математичні функції
 * @author ServoCore Team
 * @date 2025
 *
 * Оптимізовані математичні операції для embedded систем
 */

#ifndef SERVOCORE_UTIL_MATH_H
#define SERVOCORE_UTIL_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/

#define UTIL_PI       3.14159265358979323846f
#define UTIL_TWO_PI   6.28318530717958647692f
#define UTIL_HALF_PI  1.57079632679489661923f

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Обмеження значення в межах [min, max]
 *
 * @param value Значення
 * @param min Мінімум
 * @param max Максимум
 * @return float Обмежене значення
 */
float Math_Clamp(float value, float min, float max);

/**
 * @brief Обмеження int значення в межах [min, max]
 *
 * @param value Значення
 * @param min Мінімум
 * @param max Максимум
 * @return int32_t Обмежене значення
 */
int32_t Math_ClampInt(int32_t value, int32_t min, int32_t max);

/**
 * @brief Лінійна інтерполяція
 *
 * @param a Початкове значення
 * @param b Кінцеве значення
 * @param t Коефіцієнт (0.0 - 1.0)
 * @return float Інтерпольоване значення
 */
float Math_Lerp(float a, float b, float t);

/**
 * @brief Мапування значення з одного діапазону в інший
 *
 * @param value Вхідне значення
 * @param in_min Мінімум вхідного діапазону
 * @param in_max Максимум вхідного діапазону
 * @param out_min Мінімум вихідного діапазону
 * @param out_max Максимум вихідного діапазону
 * @return float Mapped значення
 */
float Math_Map(float value, float in_min, float in_max, float out_min, float out_max);

/**
 * @brief Абсолютне значення float
 *
 * @param value Значення
 * @return float Абсолютне значення
 */
float Math_Abs(float value);

/**
 * @brief Абсолютне значення int
 *
 * @param value Значення
 * @return int32_t Абсолютне значення
 */
int32_t Math_AbsInt(int32_t value);

/**
 * @brief Знак числа
 *
 * @param value Значення
 * @return float 1.0 якщо додатнє, -1.0 якщо від'ємне, 0.0 якщо нуль
 */
float Math_Sign(float value);

/**
 * @brief Мінімум з двох значень
 *
 * @param a Перше значення
 * @param b Друге значення
 * @return float Мінімальне значення
 */
float Math_Min(float a, float b);

/**
 * @brief Максимум з двох значень
 *
 * @param a Перше значення
 * @param b Друге значення
 * @return float Максимальне значення
 */
float Math_Max(float a, float b);

/**
 * @brief Нормалізація кута в діапазон [0, 360)
 *
 * @param angle Кут в градусах
 * @return float Нормалізований кут
 */
float Math_NormalizeAngle(float angle);

/**
 * @brief Нормалізація кута в діапазон [-180, 180)
 *
 * @param angle Кут в градусах
 * @return float Нормалізований кут
 */
float Math_NormalizeAngleSigned(float angle);

/**
 * @brief Найкоротша різниця між двома кутами
 *
 * @param angle1 Перший кут (градуси)
 * @param angle2 Другий кут (градуси)
 * @return float Різниця в діапазоні [-180, 180]
 */
float Math_AngleDifference(float angle1, float angle2);

/**
 * @brief Конвертація градусів у радіани
 *
 * @param degrees Градуси
 * @return float Радіани
 */
float Math_DegToRad(float degrees);

/**
 * @brief Конвертація радіанів у градуси
 *
 * @param radians Радіани
 * @return float Градуси
 */
float Math_RadToDeg(float radians);

/**
 * @brief Швидкий квадратний корінь (апроксимація)
 *
 * @param value Значення
 * @return float Квадратний корінь
 */
float Math_FastSqrt(float value);

/**
 * @brief Квадрат числа
 *
 * @param value Значення
 * @return float Квадрат
 */
float Math_Square(float value);

/**
 * @brief Степінь числа
 *
 * @param base Основа
 * @param exponent Показник степеня (int)
 * @return float Результат
 */
float Math_Pow(float base, int32_t exponent);

/**
 * @brief Порівняння float з точністю epsilon
 *
 * @param a Перше значення
 * @param b Друге значення
 * @param epsilon Точність (за замовчуванням 0.0001f)
 * @return bool true якщо приблизно рівні
 */
bool Math_FloatEquals(float a, float b, float epsilon);

/**
 * @brief Перевірка чи значення близьке до нуля
 *
 * @param value Значення
 * @param epsilon Точність
 * @return bool true якщо близько до нуля
 */
bool Math_IsZero(float value, float epsilon);

/**
 * @brief Швидке ділення на ступінь 2
 *
 * @param value Значення
 * @param shift Ступінь (наприклад 1 = /2, 2 = /4, 3 = /8)
 * @return int32_t Результат
 */
int32_t Math_FastDivPow2(int32_t value, uint8_t shift);

/**
 * @brief Швидке множення на ступінь 2
 *
 * @param value Значення
 * @param shift Ступінь (наприклад 1 = *2, 2 = *4, 3 = *8)
 * @return int32_t Результат
 */
int32_t Math_FastMulPow2(int32_t value, uint8_t shift);

/**
 * @brief Обчислення середнього значення масиву
 *
 * @param array Масив значень
 * @param size Розмір масиву
 * @return float Середнє значення
 */
float Math_Average(const float* array, uint32_t size);

/**
 * @brief Обчислення стандартного відхилення масиву
 *
 * @param array Масив значень
 * @param size Розмір масиву
 * @param mean Середнє значення (якщо вже обчислене, інакше передати 0)
 * @return float Стандартне відхилення
 */
float Math_StdDev(const float* array, uint32_t size, float mean);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_UTIL_MATH_H */
