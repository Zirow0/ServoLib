/**
 * @file math.c
 * @brief Реалізація математичних функцій
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "../../Inc/util/math.h"
#include <math.h>

/* Exported functions --------------------------------------------------------*/

float Math_Clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int32_t Math_ClampInt(int32_t value, int32_t min, int32_t max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float Math_Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float Math_Map(float value, float in_min, float in_max, float out_min, float out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float Math_Abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

int32_t Math_AbsInt(int32_t value)
{
    return (value < 0) ? -value : value;
}

float Math_Sign(float value)
{
    if (value > 0.0f) return 1.0f;
    if (value < 0.0f) return -1.0f;
    return 0.0f;
}

float Math_Min(float a, float b)
{
    return (a < b) ? a : b;
}

float Math_Max(float a, float b)
{
    return (a > b) ? a : b;
}

float Math_NormalizeAngle(float angle)
{
    while (angle >= 360.0f) {
        angle -= 360.0f;
    }
    while (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

float Math_NormalizeAngleSigned(float angle)
{
    angle = Math_NormalizeAngle(angle);
    if (angle > 180.0f) {
        angle -= 360.0f;
    }
    return angle;
}

float Math_AngleDifference(float angle1, float angle2)
{
    float diff = angle2 - angle1;
    return Math_NormalizeAngleSigned(diff);
}

float Math_DegToRad(float degrees)
{
    return degrees * (UTIL_PI / 180.0f);
}

float Math_RadToDeg(float radians)
{
    return radians * (180.0f / UTIL_PI);
}

float Math_FastSqrt(float value)
{
    // Використовуємо стандартну функцію
    // Для більшої швидкості можна використати апроксимацію
    return sqrtf(value);
}

float Math_Square(float value)
{
    return value * value;
}

float Math_Pow(float base, int32_t exponent)
{
    if (exponent == 0) return 1.0f;
    if (exponent == 1) return base;

    float result = 1.0f;
    bool negative = (exponent < 0);
    int32_t abs_exp = Math_AbsInt(exponent);

    for (int32_t i = 0; i < abs_exp; i++) {
        result *= base;
    }

    return negative ? (1.0f / result) : result;
}

bool Math_FloatEquals(float a, float b, float epsilon)
{
    return Math_Abs(a - b) < epsilon;
}

bool Math_IsZero(float value, float epsilon)
{
    return Math_Abs(value) < epsilon;
}

int32_t Math_FastDivPow2(int32_t value, uint8_t shift)
{
    return value >> shift;
}

int32_t Math_FastMulPow2(int32_t value, uint8_t shift)
{
    return value << shift;
}

float Math_Average(const float* array, uint32_t size)
{
    if (array == NULL || size == 0) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        sum += array[i];
    }

    return sum / (float)size;
}

float Math_StdDev(const float* array, uint32_t size, float mean)
{
    if (array == NULL || size == 0) {
        return 0.0f;
    }

    // Обчислення середнього якщо не передано
    if (mean == 0.0f) {
        mean = Math_Average(array, size);
    }

    // Обчислення дисперсії
    float variance = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        float diff = array[i] - mean;
        variance += diff * diff;
    }
    variance /= (float)size;

    // Стандартне відхилення = корінь з дисперсії
    return Math_FastSqrt(variance);
}
