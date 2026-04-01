/**
 * @file checksum.h
 * @brief Алгоритм CRC-8 для AEAT-9922
 * @author ServoCore Team
 * @date 2025
 *
 * CRC-8 Algorithm (поліном 0x07):
 * - Polynomial: x^8 + x^2 + x^1 + x^0 = 0x07
 * - Initial value: 0x00
 * - MSB-first (Most Significant Bit)
 * - No final XOR
 *
 * Використання:
 * @code
 * uint8_t rx_data[3];  // [DATA0, DATA1, CRC]
 * uint8_t crc = Checksum_CRC8(rx_data, 2, 0x07);
 * if (crc == rx_data[2]) {
 *     // CRC правильна
 * }
 * @endcode
 */

#ifndef SERVOCORE_UTIL_CHECKSUM_H
#define SERVOCORE_UTIL_CHECKSUM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported defines ----------------------------------------------------------*/

#define CRC8_POLY_DEFAULT       0x07    /**< CRC-8/SMBUS поліном (x^8+x^2+x+1) */

#define CRC6_POLY_AEAT9922      0x04    /**< CRC-6 поліном для AEAT-9922 RX регістрових фреймів */
#define CRC6_INIT_AEAT9922      0x04    /**< Початкове значення CRC-6 для AEAT-9922 */

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Обчислення CRC-8
 *
 * @param data Вказівник на дані для обчислення
 * @param len Довжина даних в байтах
 * @param polynomial Поліном CRC-8 (за замовчуванням 0x07)
 * @return uint8_t CRC-8 контрольна сума
 *
 * Алгоритм:
 * 1. Ініціалізувати CRC = 0x00
 * 2. Для кожного байта:
 *    - XOR з CRC
 *    - Для кожного біта:
 *      - Якщо MSB=1: зсув вліво і XOR з поліномом
 *      - Якщо MSB=0: зсув вліво
 * 3. Повернути CRC
 */
uint8_t Checksum_CRC8(const uint8_t* data, uint8_t len, uint8_t polynomial);

/**
 * @brief Обчислення CRC-6 для AEAT-9922 RX фреймів
 *
 * @param data Вказівник на дані
 * @param len Довжина даних в байтах
 * @param polynomial Поліном CRC-6 (для AEAT-9922: CRC6_POLY_AEAT9922 = 0x04)
 * @return uint8_t CRC-6 (значення в діапазоні 0x00..0x3F)
 *
 * Параметри (AEAT-9922 SPI4-B):
 *   - Polynomial: 0x04 (x^6 + x^2)
 *   - Initial value: 0x04 (CRC6_INIT_AEAT9922)
 *   - MSB-first, no final XOR
 *
 * Використання для регістрових фреймів:
 * @code
 * // RX: [DATA(8) | W(1)|E(1)|CRC6(6) | 0x00(8)]
 * uint8_t crc = Checksum_CRC6(&rx_data[0], 1, CRC6_POLY_AEAT9922);
 * if (crc != (rx_data[1] & 0x3F)) { // помилка CRC }
 * @endcode
 */
uint8_t Checksum_CRC6(const uint8_t* data, uint8_t len, uint8_t polynomial);

/**
 * @brief Обчислення біта парності (Even Parity)
 *
 * @param data Вказівник на дані для обчислення
 * @param num_bits Кількість біт для обчислення парності
 * @return uint8_t Біт парності (0 або 1)
 *
 * Алгоритм Even Parity:
 * 1. Підрахувати кількість одиниць у вказаній кількості біт
 * 2. Якщо кількість парна → повернути 0
 * 3. Якщо кількість непарна → повернути 1
 *
 * Використання в SPI4-A (AEAT-9922):
 * @code
 * uint8_t rx_data[2];  // [DATA0, DATA1]
 * // Витягти біт парності з rx_data[0] біт 7
 * uint8_t received_parity = (rx_data[0] >> 7) & 0x01;
 *
 * // Обчислити парність для 16 біт (всі біти включно з P)
 * uint16_t full_data = ((uint16_t)rx_data[0] << 8) | rx_data[1];
 * uint8_t calculated_parity = Checksum_EvenParity16(full_data);
 *
 * if (calculated_parity == 0) {
 *     // Парність правильна (парна кількість одиниць)
 * }
 * @endcode
 */
uint8_t Checksum_EvenParity16(uint16_t data);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_UTIL_CHECKSUM_H */
