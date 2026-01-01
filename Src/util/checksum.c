/**
 * @file checksum.c
 * @brief Реалізація алгоритму CRC-8
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "util/checksum.h"
#include <stddef.h>

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Обчислення CRC-8 (bit-by-bit алгоритм)
 *
 * Приклад обчислення для data = {0xA5, 0x3C}, polynomial = 0x07:
 *
 * Початок: crc = 0x00
 *
 * Байт 0 (0xA5):
 *   crc ^= 0xA5 → crc = 0xA5 (0b10100101)
 *   Біт 0: MSB=1 → crc = (0xA5 << 1) ^ 0x07 = 0x4A ^ 0x07 = 0x4D
 *   Біт 1: MSB=0 → crc = 0x4D << 1 = 0x9A
 *   Біт 2: MSB=1 → crc = (0x9A << 1) ^ 0x07 = 0x34 ^ 0x07 = 0x33
 *   Біт 3: MSB=0 → crc = 0x33 << 1 = 0x66
 *   Біт 4: MSB=0 → crc = 0x66 << 1 = 0xCC
 *   Біт 5: MSB=1 → crc = (0xCC << 1) ^ 0x07 = 0x98 ^ 0x07 = 0x9F
 *   Біт 6: MSB=1 → crc = (0x9F << 1) ^ 0x07 = 0x3E ^ 0x07 = 0x39
 *   Біт 7: MSB=0 → crc = 0x39 << 1 = 0x72
 *
 * Байт 1 (0x3C):
 *   crc ^= 0x3C → crc = 0x72 ^ 0x3C = 0x4E
 *   Біт 0: MSB=0 → crc = 0x4E << 1 = 0x9C
 *   Біт 1: MSB=1 → crc = (0x9C << 1) ^ 0x07 = 0x38 ^ 0x07 = 0x3F
 *   Біт 2: MSB=0 → crc = 0x3F << 1 = 0x7E
 *   Біт 3: MSB=0 → crc = 0x7E << 1 = 0xFC
 *   Біт 4: MSB=1 → crc = (0xFC << 1) ^ 0x07 = 0xF8 ^ 0x07 = 0xFF
 *   Біт 5: MSB=1 → crc = (0xFF << 1) ^ 0x07 = 0xFE ^ 0x07 = 0xF9
 *   Біт 6: MSB=1 → crc = (0xF9 << 1) ^ 0x07 = 0xF2 ^ 0x07 = 0xF5
 *   Біт 7: MSB=1 → crc = (0xF5 << 1) ^ 0x07 = 0xEA ^ 0x07 = 0xED
 *
 * Результат: CRC = 0xED
 */
uint8_t Checksum_CRC8(const uint8_t* data, uint8_t len, uint8_t polynomial)
{
    if (data == NULL || len == 0) {
        return 0x00;
    }

    uint8_t crc = 0x00;  // Початкове значення

    // Обробити кожен байт даних
    for (uint8_t i = 0; i < len; i++) {
        // XOR поточного байта з CRC
        crc ^= data[i];

        // Обробити 8 бітів (MSB-first)
        for (uint8_t bit = 0; bit < 8; bit++) {
            // Перевірити старший біт (MSB)
            if (crc & 0x80) {
                // Якщо MSB=1: зсув вліво і XOR з поліномом
                crc = (crc << 1) ^ polynomial;
            } else {
                // Якщо MSB=0: просто зсув вліво
                crc = (crc << 1);
            }
        }
    }

    return crc;
}
