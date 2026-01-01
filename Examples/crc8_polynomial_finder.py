#!/usr/bin/env python3
"""
CRC-8 Polynomial Finder
Автор: ServoCore Team
Дата: 2025

Скрипт для знаходження поліному CRC-8 методом перебору.
Приймає 2 байти даних і очікуваний CRC, повертає всі можливі поліноми.

Використання:
    python crc8_polynomial_finder.py <data0> <data1> <expected_crc>

Приклад:
    python crc8_polynomial_finder.py 0x40 0x3F 0xAB
    python crc8_polynomial_finder.py 64 63 171
"""

import sys


def calculate_crc8(data, polynomial, init_value=0x00):
    """
    Обчислення CRC-8 (MSB-first алгоритм)

    Args:
        data: Список байтів для обчислення
        polynomial: Поліном CRC-8 (0x00-0xFF)
        init_value: Початкове значення (за замовчуванням 0x00)

    Returns:
        CRC-8 контрольна сума (uint8)
    """
    crc = init_value

    for byte in data:
        # XOR поточного байта з CRC
        crc ^= byte

        # Обробити 8 бітів (MSB-first)
        for _ in range(8):
            if crc & 0x80:  # Перевірити старший біт
                crc = ((crc << 1) ^ polynomial) & 0xFF
            else:
                crc = (crc << 1) & 0xFF

    return crc


def find_polynomials(data, expected_crc, init_value=0x00):
    """
    Знайти всі можливі поліноми для заданих даних і CRC

    Args:
        data: Список байтів даних
        expected_crc: Очікуване значення CRC
        init_value: Початкове значення (за замовчуванням 0x00)

    Returns:
        Список знайдених поліномів
    """
    found_polynomials = []

    # Перебрати всі можливі поліноми (0x00-0xFF)
    for poly in range(0x00, 0x100):
        calculated_crc = calculate_crc8(data, poly, init_value)

        if calculated_crc == expected_crc:
            found_polynomials.append(poly)

    return found_polynomials


def main():
    print("=" * 70)
    print("CRC-8 Polynomial Finder")
    print("=" * 70)

    # Парсинг аргументів командного рядка
    if len(sys.argv) < 4:
        print("\nВикористання:")
        print("  python crc8_polynomial_finder.py <data0> <data1> <expected_crc>")
        print("\nПриклади:")
        print("  python crc8_polynomial_finder.py 0x40 0x3F 0xAB")
        print("  python crc8_polynomial_finder.py 64 63 171")
        print("\nАбо введіть дані вручну:")

        try:
            data0_input = input("Data[0] (hex або dec): ").strip()
            data1_input = input("Data[1] (hex або dec): ").strip()
            crc_input = input("Expected CRC (hex або dec): ").strip()
        except (KeyboardInterrupt, EOFError):
            print("\n\nПерервано користувачем.")
            return
    else:
        data0_input = sys.argv[1]
        data1_input = sys.argv[2]
        crc_input = sys.argv[3]

    # Конвертація вводу (підтримка hex та decimal)
    try:
        data0 = int(data0_input, 0)  # auto-detect base
        data1 = int(data1_input, 0)
        expected_crc = int(crc_input, 0)
    except ValueError:
        print(f"\nПомилка: Невірний формат вводу!")
        print(f"Data[0]: {data0_input}, Data[1]: {data1_input}, CRC: {crc_input}")
        return

    # Перевірка діапазону
    if not (0 <= data0 <= 255 and 0 <= data1 <= 255 and 0 <= expected_crc <= 255):
        print("\nПомилка: Всі значення мають бути в діапазоні 0-255 (0x00-0xFF)")
        return

    print(f"\nВхідні дані:")
    print(f"  Data[0]:       0x{data0:02X} ({data0:3d})")
    print(f"  Data[1]:       0x{data1:02X} ({data1:3d})")
    print(f"  Expected CRC:  0x{expected_crc:02X} ({expected_crc:3d})")

    # Знайти поліноми
    print(f"\nПошук поліномів (перебір 0x00-0xFF)...")
    data = [data0, data1]
    found = find_polynomials(data, expected_crc)

    print(f"\n{'=' * 70}")
    print(f"Результати:")
    print(f"{'=' * 70}")

    if found:
        print(f"\nЗнайдено {len(found)} можливих поліномів:\n")

        # Відомі поліноми для порівняння
        known_polynomials = {
            0x07: "CRC-8-CCITT (x^8 + x^2 + x^1 + 1)",
            0x31: "CRC-8-MAXIM (Dallas/Maxim)",
            0x1D: "CRC-8-SAE J1850",
            0x9B: "CRC-8-WCDMA",
            0xD5: "CRC-8-DVB-S2",
            0x2F: "CRC-8 (поліном 0x2F)",
            0x97: "CRC-8 (поліном 0x97)",
        }

        for poly in found:
            name = known_polynomials.get(poly, "Невідомий")
            print(f"  0x{poly:02X} ({poly:3d})  -  {name}")

            # Тестування на додаткових даних для валідації
            test_data = [[0x00, 0x00], [0xFF, 0xFF], [0xA5, 0x3C]]
            print(f"    Тестові CRC:")
            for td in test_data:
                test_crc = calculate_crc8(td, poly)
                print(f"      Data=[0x{td[0]:02X}, 0x{td[1]:02X}] → CRC=0x{test_crc:02X}")
            print()

        print(f"{'=' * 70}")
        print(f"\nРекомендації:")
        print(f"  1. Якщо знайдено 0x07 - це CRC-8-CCITT (найпоширеніший)")
        print(f"  2. Якщо знайдено 0x31 - це CRC-8-MAXIM")
        print(f"  3. Протестуйте знайдені поліноми на інших зразках даних")
        print(f"  4. Зверніться до datasheet для точної специфікації")

    else:
        print("\nПоліномів НЕ знайдено!")
        print("\nМожливі причини:")
        print("  1. Невірні вхідні дані")
        print("  2. CRC використовує init_value != 0x00")
        print("  3. CRC використовує final XOR")
        print("  4. Алгоритм LSB-first (а не MSB-first)")
        print("\nСпробуйте:")
        print("  - Перевірити дані з datasheet")
        print("  - Змінити init_value в коді скрипта")

    print(f"\n{'=' * 70}\n")


# Додаткові функції для розширеного тестування
def test_with_different_init_values(data, expected_crc):
    """
    Тестування з різними init_value
    """
    print("\nРозширене тестування з різними init_value:")
    init_values = [0x00, 0xFF, 0xAA, 0x55]

    for init_val in init_values:
        found = find_polynomials(data, expected_crc, init_val)
        if found:
            print(f"\n  init_value = 0x{init_val:02X}:")
            for poly in found:
                print(f"    Polynomial: 0x{poly:02X}")


if __name__ == "__main__":
    main()
