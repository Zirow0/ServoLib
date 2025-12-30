# AEAT-9922 Driver Implementation Guide

**Дата:** 2025
**Версія:** 1.0
**Автор:** ServoCore Team, КПІ ім. Ігоря Сікорського

---

## Зміст

1. [Огляд енкодера](#1-огляд-енкодера)
2. [Структура драйвера](#2-структура-драйвера)
3. [Апаратне підключення](#3-апаратне-підключення)
4. [Налаштування CubeMX](#4-налаштування-cubemx)
5. [Реалізація драйвера](#5-реалізація-драйвера)
6. [Приклад використання](#6-приклад-використання)
7. [Калібрування та налаштування](#7-калібрування-та-налаштування)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Огляд енкодера

**AEAT-9922** - це високоточний магнітний енкодер з абсолютним та інкрементальним виходами від компанії Broadcom.

### Ключові характеристики:

| Параметр | Значення |
|----------|----------|
| **Тип** | Абсолютний магнітний енкодер |
| **Роздільна здатність** | 10-18 біт (абсолютна), 1-10000 CPR (інкрементальна) |
| **Інтерфейси** | SPI (4-провідний), SSI, PWM, UVW, Інкрементальний (ABI) |
| **Точність** | ±0.1° @ 25°C (on-axis) |
| **Напруга живлення** | 3.0-5.5V |
| **Струм споживання** | ~25 mA @ 5V |
| **Температурний діапазон** | -40°C до +125°C |
| **Максимальна частота ABI** | 1 MHz |
| **SPI частота** | до 10 MHz |

### Переваги:

- ✅ Повний кут обертання (360°) без dead zones
- ✅ Одночасний вихід абсолютної та інкрементальної інформації
- ✅ Програмувана роздільна здатність
- ✅ Вбудоване калібрування
- ✅ Fail-safe режим з детекцією магнітного поля
- ✅ Низьке енергоспоживання

---

## 2. Структура драйвера

Драйвер AEAT-9922 інтегрується в архітектуру ServoLib згідно з шаровою моделлю:

```
Application Layer
    ↓
ctrl/servo.c (Servo Controller)
    ↓
drv/position/position.h (Position Sensor Interface)
    ↓
drv/position/aeat9922.h (AEAT-9922 Driver) ← ВИ ТУТ
    ↓
hwd/hwd_spi.h (SPI Abstraction)
    ↓
Board/STM32F411/hwd_spi.c (HAL Implementation)
```

### Файлова структура:

```
ServoLib/
├── Inc/
│   ├── hwd/
│   │   └── hwd_spi.h           # (створити) SPI абстракція HWD
│   └── drv/
│       └── sensor/
│           └── aeat9922.h      # (створити) Заголовок драйвера AEAT-9922
│
├── Src/
│   └── drv/
│       └── sensor/
│           └── aeat9922.c      # (створити) Реалізація драйвера
│
└── Board/
    └── STM32F411/
        └── hwd_spi.c           # (створити) SPI реалізація через HAL
```

### Основні компоненти драйвера:

1. **HWD SPI Layer** (`hwd/hwd_spi.h`, `Board/STM32F411/hwd_spi.c`)
   - Абстракція SPI інтерфейсу
   - Незалежна від конкретного енкодера

2. **AEAT-9922 Driver** (`drv/position/aeat9922.h`, `aeat9922.c`)
   - Ініціалізація та конфігурація
   - Зчитування абсолютної позиції через SPI
   - Робота з інкрементальним виходом
   - Калібрування та діагностика

3. **Position Sensor Interface** (`drv/position/position.h`)
   - Уніфікований інтерфейс для всіх датчиків положення
   - AEAT-9922 реалізує цей інтерфейс

---

## 3. Апаратне підключення

### 3.1 Схема підключення

```
STM32F411               AEAT-9922
---------               ---------
PA5 (SPI1_SCK)    →     M2 (Pin 11) - SPI Clock
PA6 (SPI1_MISO)   ←     M3 (Pin 12) - SPI MISO
PA7 (SPI1_MOSI)   →     M1 (Pin 10) - SPI MOSI
PA4 (GPIO_CS)     →     M0 (Pin 9)  - SPI CS
PB0 (GPIO)        →     MSEL (Pin 19) - Mode Select = HIGH

Опціонально (інкрементальний вихід):
PA0 (TIM2_CH1)    ←     A (Pin 24) - Quadrature A
PA1 (TIM2_CH2)    ←     B (Pin 23) - Quadrature B
PA15 (GPIO_IT)    ←     I (Pin 20) - Index pulse

Живлення:
3.3V              →     VDD (Pin 21), VDDA (Pin 7)
GND               →     VSS (Pin 22, 25), VSSA (Pin 8)
```

**Конденсатори живлення:**
- 10 μF + 100 nF між VDD та VSS (якомога ближче)
- 10 μF + 100 nF між VDDA та VSSA (якомога ближче)

### 3.2 Магнітне розміщення (On-Axis)

```
        [AEAT-9922 Chip]
              ↑
         (1 mm ± 0.25 mm)
              ↑
        [Disk Magnet]
        Ø 6mm x 2.5mm
       Діаметрально
      намагнічений N-S
```

**Вимоги до магніту:**
- Матеріал: NdFeB (grade N35SH)
- Діаметр: 6 mm (рекомендовано)
- Товщина: 2.5 mm
- Полюси: 2-полюсний (1 pole pair)
- Магнітне поле: 45-100 mT
- Відстань від чіпа: 1.0 mm ± 0.25 mm
- Центрування: ±0.5 mm максимум

---

## 4. Налаштування CubeMX

### 4.1 SPI1 для зв'язку з AEAT-9922

**Connectivity → SPI1:**
- Mode: Full-Duplex Master
- Hardware NSS Signal: Disable (використовуємо GPIO)

**Parameter Settings:**
- Frame Format: Motorola
- Data Size: 8 Bits
- First Bit: MSB First
- Prescaler: 8 (12.5 MHz при 100 MHz APB2)
- Clock Polarity (CPOL): Low
- Clock Phase (CPHA): 2 Edge
- CRC Calculation: Disabled
- NSS Signal Type: Software

**GPIO Settings:**
- PA5: SPI1_SCK (Alternate Function Push Pull, High, No Pull)
- PA6: SPI1_MISO (Alternate Function Push Pull, High, No Pull)
- PA7: SPI1_MOSI (Alternate Function Push Pull, High, No Pull)
- PA4: GPIO_Output (CS_ENCODER, Output Push Pull, High, Pull-up)
- PB0: GPIO_Output (MSEL_ENCODER, Output Push Pull, High, No Pull)

### 4.2 TIM2 для інкрементального виходу (опціонально)

**Timers → TIM2:**
- Combined Channels: Encoder Mode
- Encoder Mode: TI1 and TI2
- Counter Period: 65535 (або більше для підрахунку обертів)

**Parameter Settings:**
- Counter Mode: Up
- Prescaler: 0
- Period: 0xFFFFFFFF (максимум для 32-біт таймера)
- Input Filter: 10 (для фільтрації шумів)

**GPIO Settings:**
- PA0: TIM2_CH1 (Alternate Function, No Pull)
- PA1: TIM2_CH2 (Alternate Function, No Pull)

### 4.3 GPIO для Index (опціонально)

**GPIO:**
- PA15: GPIO_EXTI15 (External Interrupt, Rising edge, Pull-down)

**NVIC Settings:**
- EXTI line[15:10] interrupts: Enabled
- Priority: 5 (lower than critical tasks)

---

## 5. Реалізація драйвера

### 5.1 HWD SPI Layer

#### `Inc/hwd/hwd_spi.h`

```c
#ifndef HWD_SPI_H
#define HWD_SPI_H

#include "core.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Структура конфігурації SPI
 */
typedef struct {
    void* spi_handle;       // Вказівник на HAL SPI handle (наприклад, &hspi1)
    void* cs_port;          // GPIO порт для CS
    uint16_t cs_pin;        // GPIO пін для CS
    uint32_t timeout_ms;    // Таймаут операцій
} HWD_SPI_Config_t;

/**
 * @brief Ініціалізація SPI інтерфейсу
 */
Servo_Status_t HWD_SPI_Init(const HWD_SPI_Config_t* config);

/**
 * @brief Передача та прийом даних через SPI
 *
 * @param config Конфігурація SPI
 * @param tx_data Дані для передачі (NULL якщо тільки прийом)
 * @param rx_data Буфер для прийому (NULL якщо тільки передача)
 * @param size Розмір даних в байтах
 * @return Servo_Status_t Статус операції
 */
Servo_Status_t HWD_SPI_TransmitReceive(const HWD_SPI_Config_t* config,
                                        const uint8_t* tx_data,
                                        uint8_t* rx_data,
                                        uint16_t size);

/**
 * @brief Встановлення стану CS (Chip Select)
 */
void HWD_SPI_CS_Low(const HWD_SPI_Config_t* config);
void HWD_SPI_CS_High(const HWD_SPI_Config_t* config);

#endif // HWD_SPI_H
```

#### `Board/STM32F411/hwd_spi.c`

```c
#include "hwd/hwd_spi.h"
#include "stm32f4xx_hal.h"

Servo_Status_t HWD_SPI_Init(const HWD_SPI_Config_t* config)
{
    if (config == NULL || config->spi_handle == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    // Встановити CS в HIGH (неактивний)
    HWD_SPI_CS_High(config);

    return SERVO_STATUS_OK;
}

Servo_Status_t HWD_SPI_TransmitReceive(const HWD_SPI_Config_t* config,
                                        const uint8_t* tx_data,
                                        uint8_t* rx_data,
                                        uint16_t size)
{
    if (config == NULL || config->spi_handle == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    SPI_HandleTypeDef* hspi = (SPI_HandleTypeDef*)config->spi_handle;
    HAL_StatusTypeDef hal_status;

    if (tx_data != NULL && rx_data != NULL) {
        // Full duplex
        hal_status = HAL_SPI_TransmitReceive(hspi, (uint8_t*)tx_data, rx_data,
                                              size, config->timeout_ms);
    } else if (tx_data != NULL) {
        // Тільки передача
        hal_status = HAL_SPI_Transmit(hspi, (uint8_t*)tx_data, size,
                                       config->timeout_ms);
    } else if (rx_data != NULL) {
        // Тільки прийом
        hal_status = HAL_SPI_Receive(hspi, rx_data, size, config->timeout_ms);
    } else {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    return (hal_status == HAL_OK) ? SERVO_STATUS_OK : SERVO_STATUS_ERROR_COMM;
}

void HWD_SPI_CS_Low(const HWD_SPI_Config_t* config)
{
    if (config != NULL) {
        HAL_GPIO_WritePin((GPIO_TypeDef*)config->cs_port,
                          config->cs_pin, GPIO_PIN_RESET);
    }
}

void HWD_SPI_CS_High(const HWD_SPI_Config_t* config)
{
    if (config != NULL) {
        HAL_GPIO_WritePin((GPIO_TypeDef*)config->cs_port,
                          config->cs_pin, GPIO_PIN_SET);
    }
}
```

### 5.2 AEAT-9922 Driver

#### `Inc/drv/position/aeat9922.h`

```c
#ifndef DRV_AEAT9922_H
#define DRV_AEAT9922_H

#include "core.h"
#include "drv/position/position.h"
#include "hwd/hwd_spi.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Режими інтерфейсу AEAT-9922
 */
typedef enum {
    AEAT9922_INTERFACE_SPI4_16BIT = 0,  // SPI-4(A) 16-біт з парністю
    AEAT9922_INTERFACE_SPI4_24BIT = 1   // SPI-4(B) 24-біт з CRC
} AEAT9922_Interface_t;

/**
 * @brief Роздільна здатність абсолютного виходу
 */
typedef enum {
    AEAT9922_ABS_RES_18BIT = 0,  // 262144 позицій
    AEAT9922_ABS_RES_17BIT = 1,  // 131072 позицій
    AEAT9922_ABS_RES_16BIT = 2,  // 65536 позицій
    AEAT9922_ABS_RES_15BIT = 3,  // 32768 позицій
    AEAT9922_ABS_RES_14BIT = 4,  // 16384 позицій
    AEAT9922_ABS_RES_13BIT = 5,  // 8192 позицій
    AEAT9922_ABS_RES_12BIT = 6,  // 4096 позицій
    AEAT9922_ABS_RES_11BIT = 7,  // 2048 позицій
    AEAT9922_ABS_RES_10BIT = 8   // 1024 позицій
} AEAT9922_Abs_Resolution_t;

/**
 * @brief Структура статусу енкодера
 */
typedef struct {
    bool ready;          // RDY: Енкодер готовий
    bool magnet_high;    // MHI: Магніт занадто близько
    bool magnet_low;     // MLO: Магніт занадто далеко
    bool memory_error;   // MEM_Err: Помилка пам'яті EEPROM
} AEAT9922_Status_t;

/**
 * @brief Конфігурація AEAT-9922
 */
typedef struct {
    // SPI конфігурація
    HWD_SPI_Config_t spi_config;

    // GPIO для MSEL піна
    void* msel_port;
    uint16_t msel_pin;

    // Налаштування роздільної здатності
    AEAT9922_Abs_Resolution_t abs_resolution;  // Абсолютна роздільність
    uint16_t incremental_cpr;                   // Інкрементальна CPR (1-10000)

    // Інтерфейс
    AEAT9922_Interface_t interface_mode;

    // Напрямок обертання (true = CCW count up)
    bool direction_ccw;

    // Параметри інкрементального виходу (опціонально)
    bool enable_incremental;      // Використовувати апаратний підрахунок
    void* encoder_timer_handle;   // Вказівник на TIM handle для encoder mode

} AEAT9922_Config_t;

/**
 * @brief Структура драйвера AEAT-9922
 */
typedef struct {
    AEAT9922_Config_t config;
    Sensor_Interface_t interface;  // Реалізація Sensor_Interface_t

    // Дані датчика
    uint32_t raw_position;         // Сира позиція з енкодера
    float angle_degrees;           // Кут в градусах (0-360)
    float velocity;                // Швидкість (град/с)
    uint32_t revolution_count;     // Лічильник повних обертів (через Index)

    // Статус
    AEAT9922_Status_t status;
    uint32_t error_count;

    // Часові мітки для обчислення швидкості
    uint32_t last_update_time;
    float last_angle;

    // Інкрементальний лічильник (якщо використовується)
    int32_t incremental_count;
    int32_t last_incremental_count;

} AEAT9922_Driver_t;

/**
 * @brief Створення драйвера AEAT-9922
 */
Servo_Status_t AEAT9922_Create(AEAT9922_Driver_t* driver,
                                const AEAT9922_Config_t* config);

/**
 * @brief Ініціалізація AEAT-9922
 */
Servo_Status_t AEAT9922_Init(void* driver);

/**
 * @brief Зчитування абсолютного кута через SPI
 */
Servo_Status_t AEAT9922_ReadAngle(void* driver, float* angle);

/**
 * @brief Обчислення швидкості
 */
Servo_Status_t AEAT9922_GetVelocity(void* driver, float* velocity);

/**
 * @brief Зчитування статусу енкодера
 */
Servo_Status_t AEAT9922_ReadStatus(AEAT9922_Driver_t* driver);

/**
 * @brief Зчитування регістру через SPI
 */
Servo_Status_t AEAT9922_ReadRegister(AEAT9922_Driver_t* driver,
                                      uint8_t address, uint8_t* value);

/**
 * @brief Запис регістру через SPI
 */
Servo_Status_t AEAT9922_WriteRegister(AEAT9922_Driver_t* driver,
                                       uint8_t address, uint8_t value);

/**
 * @brief Розблокування регістрів для запису
 */
Servo_Status_t AEAT9922_UnlockRegisters(AEAT9922_Driver_t* driver);

/**
 * @brief Програмування EEPROM (збереження конфігурації)
 */
Servo_Status_t AEAT9922_ProgramEEPROM(AEAT9922_Driver_t* driver);

/**
 * @brief Калібрування точності
 */
Servo_Status_t AEAT9922_CalibrateAccuracy(AEAT9922_Driver_t* driver);

/**
 * @brief Калібрування нульової позиції
 */
Servo_Status_t AEAT9922_CalibrateZero(AEAT9922_Driver_t* driver);

/**
 * @brief Оновлення інкрементального лічильника (якщо використовується)
 */
Servo_Status_t AEAT9922_UpdateIncrementalCount(AEAT9922_Driver_t* driver);

/**
 * @brief Callback для Index pulse (якщо використовується)
 */
void AEAT9922_IndexPulseCallback(AEAT9922_Driver_t* driver);

// Адреси регістрів AEAT-9922
#define AEAT9922_REG_CONFIG0            0x07
#define AEAT9922_REG_CONFIG1            0x08
#define AEAT9922_REG_INC_RES_HIGH       0x09
#define AEAT9922_REG_INC_RES_LOW        0x0A
#define AEAT9922_REG_CONFIG2            0x0B
#define AEAT9922_REG_ZERO_HIGH          0x0C
#define AEAT9922_REG_ZERO_MID           0x0D
#define AEAT9922_REG_ZERO_LOW           0x0E
#define AEAT9922_REG_UNLOCK             0x10
#define AEAT9922_REG_PROGRAM            0x11
#define AEAT9922_REG_CALIBRATE          0x12
#define AEAT9922_REG_STATUS             0x21
#define AEAT9922_REG_CALIB_STATUS       0x22
#define AEAT9922_REG_POSITION           0x3F

// Константи
#define AEAT9922_UNLOCK_CODE            0xAB
#define AEAT9922_PROGRAM_CODE           0xA1
#define AEAT9922_CALIB_ACCURACY_START   0x02
#define AEAT9922_CALIB_ZERO_START       0x08
#define AEAT9922_CALIB_EXIT             0x00

#define AEAT9922_POWERUP_TIME_MS        10
#define AEAT9922_EEPROM_WRITE_TIME_MS   40

#endif // DRV_AEAT9922_H
```

#### `Src/drv/position/aeat9922.c`

```c
#include "drv/position/aeat9922.h"
#include "hwd/hwd_timer.h"
#include "util/math.h"
#include <string.h>

// Допоміжні макроси для SPI команд
#define AEAT9922_CMD_READ(addr)   ((addr) & 0x7F)
#define AEAT9922_CMD_WRITE(addr)  (((addr) & 0x7F) | 0x80)

// Прототипи внутрішніх функцій
static Servo_Status_t aeat9922_spi_transaction(AEAT9922_Driver_t* driver,
                                                 uint8_t cmd,
                                                 uint8_t* data);
static uint32_t aeat9922_raw_to_position(uint32_t raw, AEAT9922_Abs_Resolution_t res);
static float aeat9922_position_to_degrees(uint32_t position, AEAT9922_Abs_Resolution_t res);

Servo_Status_t AEAT9922_Create(AEAT9922_Driver_t* driver,
                                const AEAT9922_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    // Ініціалізація структури
    memset(driver, 0, sizeof(AEAT9922_Driver_t));
    memcpy(&driver->config, config, sizeof(AEAT9922_Config_t));

    // Налаштування Sensor Interface
    driver->interface.driver = driver;
    driver->interface.init = AEAT9922_Init;
    driver->interface.read_angle = AEAT9922_ReadAngle;
    driver->interface.get_velocity = AEAT9922_GetVelocity;

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_Init(void* driver)
{
    if (driver == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)driver;
    Servo_Status_t status;

    // 1. Встановити MSEL = HIGH для SPI4 режиму
    HAL_GPIO_WritePin((GPIO_TypeDef*)enc->config.msel_port,
                      enc->config.msel_pin, GPIO_PIN_SET);

    // 2. Ініціалізація SPI
    status = HWD_SPI_Init(&enc->config.spi_config);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // 3. Зачекати Power-Up час (10 ms)
    HWD_Timer_DelayMs(AEAT9922_POWERUP_TIME_MS);

    // 4. Перевірити статус енкодера
    status = AEAT9922_ReadStatus(enc);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    if (!enc->status.ready) {
        return SERVO_STATUS_ERROR_INIT;
    }

    // 5. Розблокувати регістри
    status = AEAT9922_UnlockRegisters(enc);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // 6. Налаштувати абсолютну роздільність
    uint8_t config1;
    status = AEAT9922_ReadRegister(enc, AEAT9922_REG_CONFIG1, &config1);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    config1 = (config1 & 0xF0) | (enc->config.abs_resolution & 0x0F);

    // Додати налаштування напрямку
    if (enc->config.direction_ccw) {
        config1 |= (1 << 4);  // Біт 4: Direction (CCW count up)
    } else {
        config1 &= ~(1 << 4);
    }

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_CONFIG1, config1);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // 7. Налаштувати інкрементальну роздільність
    uint16_t cpr = enc->config.incremental_cpr;
    if (cpr < 1) cpr = 1;
    if (cpr > 10000) cpr = 10000;

    uint8_t cpr_high = (cpr >> 8) & 0x3F;
    uint8_t cpr_low = cpr & 0xFF;

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_INC_RES_HIGH, cpr_high);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_INC_RES_LOW, cpr_low);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // 8. Налаштувати PSEL для вибору інтерфейсу SPI4
    uint8_t config2;
    status = AEAT9922_ReadRegister(enc, AEAT9922_REG_CONFIG2, &config2);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    config2 = (config2 & 0x9F) | ((enc->config.interface_mode & 0x03) << 5);
    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_CONFIG2, config2);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // 9. Програмування EEPROM (опціонально, можна викликати окремо)
    // status = AEAT9922_ProgramEEPROM(enc);

    // 10. Ініціалізувати інкрементальний лічильник (якщо використовується)
    if (enc->config.enable_incremental && enc->config.encoder_timer_handle != NULL) {
        TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)enc->config.encoder_timer_handle;
        HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
        enc->incremental_count = 0;
        enc->last_incremental_count = 0;
    }

    // 11. Ініціалізувати часові мітки
    enc->last_update_time = HWD_Timer_GetMillis();

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_ReadAngle(void* driver, float* angle)
{
    if (driver == NULL || angle == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)driver;
    Servo_Status_t status;
    uint8_t data[3] = {0};

    // Зчитати абсолютну позицію (регістр 0x3F)
    uint8_t cmd = AEAT9922_CMD_READ(AEAT9922_REG_POSITION);

    // SPI транзакція
    HWD_SPI_CS_Low(&enc->config.spi_config);
    HWD_Timer_DelayUs(1);  // t_L delay (350 ns мін)

    // Відправити команду та отримати дані
    status = HWD_SPI_TransmitReceive(&enc->config.spi_config,
                                      &cmd, data, 3);

    HWD_Timer_DelayUs(1);  // t_H delay
    HWD_SPI_CS_High(&enc->config.spi_config);

    if (status != SERVO_STATUS_OK) {
        enc->error_count++;
        return status;
    }

    // Розпакувати дані в залежності від режиму
    if (enc->config.interface_mode == AEAT9922_INTERFACE_SPI4_16BIT) {
        // 16-біт режим: data[0-1] містить позицію
        enc->raw_position = ((uint32_t)data[0] << 8) | data[1];

        // Перевірка парності (data[2] містить статус)
        // TODO: Додати перевірку парності

    } else {
        // 24-біт режим: data[0-2] містить позицію + CRC
        enc->raw_position = ((uint32_t)data[0] << 10) | ((uint32_t)data[1] << 2);

        // TODO: Перевірка CRC
    }

    // Конвертувати в градуси
    enc->angle_degrees = aeat9922_position_to_degrees(enc->raw_position,
                                                       enc->config.abs_resolution);
    *angle = enc->angle_degrees;

    // Оновити швидкість
    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t dt = current_time - enc->last_update_time;

    if (dt > 0) {
        float angle_diff = enc->angle_degrees - enc->last_angle;

        // Обробка переходу через 0/360
        if (angle_diff > 180.0f) {
            angle_diff -= 360.0f;
        } else if (angle_diff < -180.0f) {
            angle_diff += 360.0f;
        }

        enc->velocity = (angle_diff * 1000.0f) / dt;  // град/с
        enc->last_angle = enc->angle_degrees;
        enc->last_update_time = current_time;
    }

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_GetVelocity(void* driver, float* velocity)
{
    if (driver == NULL || velocity == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)driver;
    *velocity = enc->velocity;

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_ReadStatus(AEAT9922_Driver_t* driver)
{
    if (driver == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    uint8_t status_reg;
    Servo_Status_t status = AEAT9922_ReadRegister(driver,
                                                   AEAT9922_REG_STATUS,
                                                   &status_reg);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    driver->status.ready = (status_reg & (1 << 7)) != 0;
    driver->status.magnet_high = (status_reg & (1 << 6)) != 0;
    driver->status.magnet_low = (status_reg & (1 << 5)) != 0;
    driver->status.memory_error = (status_reg & (1 << 4)) != 0;

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_ReadRegister(AEAT9922_Driver_t* driver,
                                      uint8_t address, uint8_t* value)
{
    if (driver == NULL || value == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    uint8_t cmd = AEAT9922_CMD_READ(address);
    uint8_t rx_data[2] = {0};

    HWD_SPI_CS_Low(&driver->config.spi_config);
    HWD_Timer_DelayUs(1);

    Servo_Status_t status = HWD_SPI_TransmitReceive(&driver->config.spi_config,
                                                     &cmd, rx_data, 2);

    HWD_Timer_DelayUs(1);
    HWD_SPI_CS_High(&driver->config.spi_config);

    if (status == SERVO_STATUS_OK) {
        *value = rx_data[1];  // Дані в другому байті
    }

    return status;
}

Servo_Status_t AEAT9922_WriteRegister(AEAT9922_Driver_t* driver,
                                       uint8_t address, uint8_t value)
{
    if (driver == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    uint8_t tx_data[2];
    tx_data[0] = AEAT9922_CMD_WRITE(address);
    tx_data[1] = value;

    HWD_SPI_CS_Low(&driver->config.spi_config);
    HWD_Timer_DelayUs(1);

    Servo_Status_t status = HWD_SPI_TransmitReceive(&driver->config.spi_config,
                                                     tx_data, NULL, 2);

    HWD_Timer_DelayUs(1);
    HWD_SPI_CS_High(&driver->config.spi_config);

    HWD_Timer_DelayMs(1);  // Час на обробку запису

    return status;
}

Servo_Status_t AEAT9922_UnlockRegisters(AEAT9922_Driver_t* driver)
{
    return AEAT9922_WriteRegister(driver, AEAT9922_REG_UNLOCK,
                                   AEAT9922_UNLOCK_CODE);
}

Servo_Status_t AEAT9922_ProgramEEPROM(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Записати код програмування
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_PROGRAM,
                                     AEAT9922_PROGRAM_CODE);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Зачекати завершення (40 ms)
    HWD_Timer_DelayMs(AEAT9922_EEPROM_WRITE_TIME_MS);

    // Перевірити статус пам'яті
    status = AEAT9922_ReadStatus(driver);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    if (driver->status.memory_error) {
        return SERVO_STATUS_ERROR_INIT;
    }

    return SERVO_STATUS_OK;
}

Servo_Status_t AEAT9922_CalibrateAccuracy(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Розблокувати регістри
    status = AEAT9922_UnlockRegisters(driver);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Запустити калібрування (магніт має обертатися 60-2000 RPM)
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_ACCURACY_START);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Зачекати завершення калібрування (~1-2 секунди)
    HWD_Timer_DelayMs(2000);

    // Перевірити статус калібрування
    uint8_t calib_status;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CALIB_STATUS,
                                    &calib_status);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Біти [1:0]: 10 = Pass, 11 = Fail
    uint8_t calib_result = calib_status & 0x03;
    if (calib_result != 0x02) {
        return SERVO_STATUS_ERROR_INIT;  // Calibration failed
    }

    // Вийти з режиму калібрування
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_EXIT);

    return status;
}

Servo_Status_t AEAT9922_CalibrateZero(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Розблокувати регістри
    status = AEAT9922_UnlockRegisters(driver);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Запустити zero reset (вал має бути нерухомий в нульовій позиції)
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_ZERO_START);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Зачекати завершення
    HWD_Timer_DelayMs(100);

    // Перевірити статус
    uint8_t calib_status;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CALIB_STATUS,
                                    &calib_status);
    if (status != SERVO_STATUS_OK) {
        return status;
    }

    // Біти [3:2]: 10 = Pass, 11 = Fail
    uint8_t calib_result = (calib_status >> 2) & 0x03;
    if (calib_result != 0x02) {
        return SERVO_STATUS_ERROR_INIT;
    }

    // Вийти
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_EXIT);

    return status;
}

Servo_Status_t AEAT9922_UpdateIncrementalCount(AEAT9922_Driver_t* driver)
{
    if (driver == NULL || !driver->config.enable_incremental) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)driver->config.encoder_timer_handle;
    if (htim == NULL) {
        return SERVO_STATUS_ERROR_INVALID_PARAM;
    }

    // Зчитати поточний лічильник з таймера
    int32_t current_count = (int32_t)__HAL_TIM_GET_COUNTER(htim);

    // Обчислити різницю
    int32_t delta = current_count - driver->last_incremental_count;

    // Оновити загальний лічильник
    driver->incremental_count += delta;
    driver->last_incremental_count = current_count;

    return SERVO_STATUS_OK;
}

void AEAT9922_IndexPulseCallback(AEAT9922_Driver_t* driver)
{
    if (driver != NULL) {
        // Підрахувати повний оберт
        driver->revolution_count++;

        // Можна також скинути інкрементальний лічильник, якщо потрібно
        // driver->incremental_count = 0;
    }
}

// ======== Допоміжні функції ========

static uint32_t aeat9922_raw_to_position(uint32_t raw, AEAT9922_Abs_Resolution_t res)
{
    // Конвертувати сире значення в залежності від роздільної здатності
    uint8_t shift = 18 - (uint8_t)res;  // 18-біт максимум
    return raw >> shift;
}

static float aeat9922_position_to_degrees(uint32_t position, AEAT9922_Abs_Resolution_t res)
{
    // Обчислити максимальне значення для даної роздільної здатності
    uint32_t max_count = (1 << (18 - (uint8_t)res));

    // Конвертувати в градуси (0-360)
    float degrees = (float)position * 360.0f / (float)max_count;

    return degrees;
}
```

---

## 6. Приклад використання

### 6.1 Базовий приклад (тільки SPI)

```c
#include "drv/position/aeat9922.h"
#include "Board/STM32F411/board_config.h"

// Глобальні змінні
extern SPI_HandleTypeDef hspi1;
AEAT9922_Driver_t encoder;

void Encoder_Example_Init(void)
{
    // Конфігурація AEAT-9922
    AEAT9922_Config_t config = {
        // SPI конфігурація
        .spi_config = {
            .spi_handle = &hspi1,
            .cs_port = GPIOA,
            .cs_pin = GPIO_PIN_4,
            .timeout_ms = 100
        },

        // MSEL пін
        .msel_port = GPIOB,
        .msel_pin = GPIO_PIN_0,

        // Роздільна здатність
        .abs_resolution = AEAT9922_ABS_RES_18BIT,  // 262144 позицій
        .incremental_cpr = 1024,                    // 1024 імпульсів/оберт

        // Інтерфейс
        .interface_mode = AEAT9922_INTERFACE_SPI4_16BIT,

        // Напрямок
        .direction_ccw = false,  // CW = count up

        // Інкрементальний вихід (вимкнено в цьому прикладі)
        .enable_incremental = false,
        .encoder_timer_handle = NULL
    };

    // Створити драйвер
    Servo_Status_t status = AEAT9922_Create(&encoder, &config);
    if (status != SERVO_STATUS_OK) {
        // Помилка створення
        Error_Handler();
    }

    // Ініціалізувати
    status = AEAT9922_Init(&encoder);
    if (status != SERVO_STATUS_OK) {
        // Помилка ініціалізації
        Error_Handler();
    }

    // Перевірити статус магніту
    AEAT9922_ReadStatus(&encoder);
    if (encoder.status.magnet_high || encoder.status.magnet_low) {
        // Магніт неправильно розташований!
        Error_Handler();
    }
}

void Encoder_Example_Loop(void)
{
    float angle, velocity;

    // Зчитати кут
    if (AEAT9922_ReadAngle(&encoder, &angle) == SERVO_STATUS_OK) {
        // Використати кут (0-360 градусів)
        printf("Angle: %.2f degrees\n", angle);
    }

    // Отримати швидкість
    if (AEAT9922_GetVelocity(&encoder, &velocity) == SERVO_STATUS_OK) {
        printf("Velocity: %.2f deg/s\n", velocity);
    }

    HAL_Delay(10);  // 100 Hz оновлення
}
```

### 6.2 Повний приклад (SPI + інкрементальний вихід)

```c
#include "drv/position/aeat9922.h"
#include "Board/STM32F411/board_config.h"

// Глобальні змінні
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
AEAT9922_Driver_t encoder;

void Encoder_Full_Init(void)
{
    AEAT9922_Config_t config = {
        .spi_config = {
            .spi_handle = &hspi1,
            .cs_port = GPIOA,
            .cs_pin = GPIO_PIN_4,
            .timeout_ms = 100
        },
        .msel_port = GPIOB,
        .msel_pin = GPIO_PIN_0,
        .abs_resolution = AEAT9922_ABS_RES_14BIT,  // 16384 позицій
        .incremental_cpr = 1024,
        .interface_mode = AEAT9922_INTERFACE_SPI4_16BIT,
        .direction_ccw = false,

        // Увімкнути інкрементальний вихід
        .enable_incremental = true,
        .encoder_timer_handle = &htim2
    };

    AEAT9922_Create(&encoder, &config);
    AEAT9922_Init(&encoder);
}

void Encoder_Full_Loop(void)
{
    float angle;

    // Зчитати абсолютний кут через SPI
    AEAT9922_ReadAngle(&encoder, &angle);

    // Оновити інкрементальний лічильник
    AEAT9922_UpdateIncrementalCount(&encoder);

    // Використати дані
    printf("Absolute: %.2f deg, Incremental: %ld counts, Revs: %lu\n",
           angle,
           encoder.incremental_count,
           encoder.revolution_count);

    HAL_Delay(10);
}

// Callback для Index pulse (налаштувати EXTI для PA15)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_15) {
        AEAT9922_IndexPulseCallback(&encoder);
    }
}
```

### 6.3 Інтеграція з ServoLib

```c
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/position/aeat9922.h"

// Глобальні змінні
Servo_Controller_t servo;
PWM_Motor_Driver_t motor;
AEAT9922_Driver_t encoder;

void ServoWithAEAT9922_Init(void)
{
    // 1. Ініціалізація енкодера AEAT-9922
    AEAT9922_Config_t encoder_config = {
        // ... (як у попередніх прикладах)
    };
    AEAT9922_Create(&encoder, &encoder_config);
    AEAT9922_Init(&encoder);

    // 2. Ініціалізація мотора
    PWM_Motor_Config_t motor_config = {
        .type = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd_timer = &htim3,
        .pwm_fwd_channel = TIM_CHANNEL_1,
        .pwm_bwd_timer = &htim3,
        .pwm_bwd_channel = TIM_CHANNEL_2
    };
    PWM_Motor_Create(&motor, &motor_config);

    Motor_Params_t motor_params = {
        .max_power = 100.0f,
        .min_power = 5.0f,
        .max_current = 2000,
        .invert_direction = false
    };
    PWM_Motor_Init(&motor, &motor_params);

    // 3. Конфігурація сервоконтролера
    Servo_Config_t servo_config = {
        .update_frequency = 1000.0f,  // 1 kHz

        // PID параметри
        .pid_position = {
            .kp = 2.0f,
            .ki = 0.1f,
            .kd = 0.05f,
            .output_min = -100.0f,
            .output_max = 100.0f
        },

        // Safety параметри
        .position_min = 0.0f,
        .position_max = 360.0f,
        .max_velocity = 180.0f,  // 180 град/с

        // Інші параметри...
    };

    // 4. Ініціалізація сервоконтролера з AEAT-9922
    Servo_Init(&servo, &servo_config, &motor.interface);

    // ВАЖЛИВО: Встановити інтерфейс датчика AEAT-9922
    servo.sensor = &encoder.interface;
}

void ServoWithAEAT9922_Loop(void)
{
    // Оновлення сервоконтролера (викликати на частоті 1 kHz)
    Servo_Update(&servo);
}

void ServoWithAEAT9922_Example(void)
{
    // Встановити цільову позицію 90°
    Servo_SetPosition(&servo, 90.0f);

    // Зачекати досягнення
    while (!Servo_IsAtTarget(&servo)) {
        Servo_Update(&servo);
        HAL_Delay(1);
    }

    // Отримати поточну позицію
    float current_pos = Servo_GetPosition(&servo);
    printf("Reached position: %.2f degrees\n", current_pos);
}
```

---

## 7. Калібрування та налаштування

### 7.1 Калібрування точності (Accuracy Calibration)

**Коли потрібно:**
- При першому встановленні магніту
- Після зміни положення магніту
- Періодично для підтримки точності

**Процедура:**

```c
void PerformAccuracyCalibration(void)
{
    printf("Starting accuracy calibration...\n");
    printf("Ensure magnet is rotating at 60-2000 RPM\n");

    HAL_Delay(2000);  // Дати час на розгін

    Servo_Status_t status = AEAT9922_CalibrateAccuracy(&encoder);

    if (status == SERVO_STATUS_OK) {
        printf("Accuracy calibration: PASS\n");

        // Зберегти в EEPROM
        AEAT9922_ProgramEEPROM(&encoder);
        printf("Configuration saved to EEPROM\n");
    } else {
        printf("Accuracy calibration: FAIL\n");
        printf("Check magnet alignment and speed\n");
    }
}
```

### 7.2 Калібрування нуля (Zero Reset)

**Коли потрібно:**
- Встановлення нової нульової точки
- Вирівнювання з механічним нулем

**Процедура:**

```c
void PerformZeroCalibration(void)
{
    printf("Starting zero calibration...\n");
    printf("Position shaft at desired zero position\n");
    printf("Press button when ready...\n");

    // Зачекати на підтвердження від користувача
    WaitForButtonPress();

    // Переконатися, що вал нерухомий
    HAL_Delay(500);

    Servo_Status_t status = AEAT9922_CalibrateZero(&encoder);

    if (status == SERVO_STATUS_OK) {
        printf("Zero calibration: PASS\n");

        // Зберегти в EEPROM
        AEAT9922_ProgramEEPROM(&encoder);
        printf("Zero position saved to EEPROM\n");
    } else {
        printf("Zero calibration: FAIL\n");
        printf("Ensure shaft is stationary\n");
    }
}
```

### 7.3 Перевірка розміщення магніту

```c
void CheckMagnetAlignment(void)
{
    AEAT9922_ReadStatus(&encoder);

    if (encoder.status.magnet_high) {
        printf("WARNING: Magnet too close (MHI flag)\n");
        printf("Recommended: Increase distance from chip\n");
    }
    else if (encoder.status.magnet_low) {
        printf("WARNING: Magnet too far (MLO flag)\n");
        printf("Recommended: Decrease distance from chip\n");
    }
    else if (encoder.status.ready) {
        printf("OK: Magnet alignment is good\n");

        // Можна додатково виміряти INL (integral non-linearity)
        // шляхом обертання валу і перевірки монотонності
    }

    if (encoder.status.memory_error) {
        printf("ERROR: EEPROM memory error\n");
        printf("Reprogram configuration\n");
    }
}
```

### 7.4 Налаштування роздільної здатності

**Вибір абсолютної роздільної здатності:**

| Застосування | Рекомендована роздільна здатність |
|--------------|-----------------------------------|
| Низька точність (±1°) | 10-12 біт (1024-4096) |
| Середня точність (±0.5°) | 14 біт (16384) |
| Висока точність (±0.1°) | 16-18 біт (65536-262144) |

**Вибір інкрементальної роздільної здатності:**

```c
// Формула: max_freq = max_RPM × CPR / 60
// Обмеження: max_freq < 1 MHz

// Приклад: max_RPM = 3000
// CPR = 1 MHz × 60 / 3000 = 20000 CPR (теоретично)
// Але AEAT-9922 обмежений до 10000 CPR максимум

uint16_t CalculateOptimalCPR(uint16_t max_rpm)
{
    // Максимальна частота 1 MHz
    uint32_t max_cpr = (1000000 * 60) / max_rpm;

    // Обмежити до 10000
    if (max_cpr > 10000) {
        max_cpr = 10000;
    }

    return (uint16_t)max_cpr;
}
```

---

## 8. Troubleshooting

### 8.1 Енкодер не відповідає (ReadAngle повертає помилку)

**Можливі причини:**
1. Неправильне підключення SPI
2. MSEL пін не встановлений в HIGH
3. Недостатнє живлення (VDD < 3.0V)
4. Погане з'єднання CS/CLK/MISO/MOSI

**Рішення:**
```c
// Діагностика SPI
void DiagnoseSPI(void)
{
    // Перевірити стан CS
    GPIO_PinState cs_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    printf("CS pin state: %d (should be HIGH when idle)\n", cs_state);

    // Перевірити MSEL
    GPIO_PinState msel_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    printf("MSEL pin state: %d (should be HIGH for SPI4)\n", msel_state);

    // Спробувати зчитати регістр статусу
    uint8_t status;
    if (AEAT9922_ReadRegister(&encoder, AEAT9922_REG_STATUS, &status) == SERVO_STATUS_OK) {
        printf("Status register: 0x%02X\n", status);
    } else {
        printf("ERROR: Cannot read status register\n");
        printf("Check SPI wiring and configuration\n");
    }
}
```

### 8.2 Магніт занадто близько/далеко (MHI/MLO flags)

**Діагностика:**
```c
void DiagnoseMagnet(void)
{
    AEAT9922_ReadStatus(&encoder);

    if (encoder.status.magnet_high) {
        printf("Magnet too close: Increase gap by 0.1-0.3 mm\n");
    }
    if (encoder.status.magnet_low) {
        printf("Magnet too far: Decrease gap by 0.1-0.3 mm\n");
    }

    // Оптимальна відстань: 1.0 mm ± 0.25 mm
    printf("Target distance: 1.0 mm from chip surface\n");
}
```

### 8.3 Нестабільні показання

**Можливі причини:**
1. Електромагнітні перешкоди
2. Недостатня фільтрація живлення
3. Магніт не центрований
4. Недостатня якість магніту

**Рішення:**
```c
// Додати програмний фільтр
float filtered_angle = 0.0f;
float alpha = 0.1f;  // Коефіцієнт фільтра (0-1)

void ReadFilteredAngle(float* angle)
{
    float raw_angle;
    if (AEAT9922_ReadAngle(&encoder, &raw_angle) == SERVO_STATUS_OK) {
        // Експоненціальний фільтр
        filtered_angle = alpha * raw_angle + (1.0f - alpha) * filtered_angle;
        *angle = filtered_angle;
    }
}
```

**Апаратне рішення:**
- Додати феритові кільця на кабелі SPI
- Використати екрановані кабелі
- Розмістити конденсатори якомога ближче до пінів VDD/VSS

### 8.4 Інкрементальний вихід не працює

**Перевірка:**
```c
void DiagnoseIncrementalOutput(void)
{
    // Перевірити, чи налаштований таймер в Encoder Mode
    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)encoder.config.encoder_timer_handle;

    if (htim == NULL) {
        printf("ERROR: Timer handle is NULL\n");
        return;
    }

    // Зчитати лічильник
    uint32_t count1 = __HAL_TIM_GET_COUNTER(htim);
    HAL_Delay(100);
    uint32_t count2 = __HAL_TIM_GET_COUNTER(htim);

    if (count1 == count2) {
        printf("WARNING: Encoder count not changing\n");
        printf("Check A/B signal connections (PA0/PA1)\n");
        printf("Check CPR configuration (current: %d)\n", encoder.config.incremental_cpr);
    } else {
        printf("OK: Encoder counting: %lu -> %lu\n", count1, count2);
    }
}
```

### 8.5 Помилка EEPROM (MEM_Err flag)

**Діагностика та виправлення:**
```c
void FixEEPROMError(void)
{
    AEAT9922_ReadStatus(&encoder);

    if (encoder.status.memory_error) {
        printf("EEPROM error detected\n");
        printf("Attempting to reprogram configuration...\n");

        // Розблокувати регістри
        AEAT9922_UnlockRegisters(&encoder);

        // Налаштувати знову
        AEAT9922_Init(&encoder);

        // Програмувати EEPROM
        if (AEAT9922_ProgramEEPROM(&encoder) == SERVO_STATUS_OK) {
            printf("EEPROM reprogrammed successfully\n");
        } else {
            printf("EEPROM programming failed - hardware issue?\n");
        }
    }
}
```

---

## Додаток A: Адреси регістрів AEAT-9922

| Адреса | Назва | Опис | R/W |
|--------|-------|------|-----|
| 0x07 | Customer Configuration-0 | ST Zero, Accuracy Cal, Axis Mode, I-Width, I-Phase | R/W |
| 0x08 | Customer Configuration-1 | Hysteresis, Direction, Absolute Resolution | R/W |
| 0x09 | Incremental Resolution High | CPR[13:8] | R/W |
| 0x0A | Incremental Resolution Low | CPR[7:0] | R/W |
| 0x0B | Customer Configuration-2 | PSEL[1:0], UVW/PWM settings | R/W |
| 0x0C | Zero Reset High | Zero offset[17:12] | R/W |
| 0x0D | Zero Reset Mid | Zero offset[11:4] | R/W |
| 0x0E | Zero Reset Low | Zero offset[3:0] | R/W |
| 0x10 | Unlock Register | Розблокування (0xAB) | W |
| 0x11 | Program EEPROM | Програмування (0xA1) | W |
| 0x12 | Calibration Control | Accuracy Cal, Zero Reset | W |
| 0x21 | Status Register | RDY, MHI, MLO, MEM_Err | R |
| 0x22 | Calibration Status | Calibration результати | R |
| 0x3F | Position Register | Абсолютна позиція | R |

---

## Додаток B: Типові помилки та попередження

### Помилки компіляції

**"undefined reference to HWD_SPI_xxx"**
- Не додано `Board/STM32F411/hwd_spi.c` до проекту
- Додати файл в Source files в STM32CubeIDE

**"unknown type name SPI_HandleTypeDef"**
- Не включено STM32 HAL заголовки
- Додати `#include "stm32f4xx_hal.h"` в `hwd_spi.c`

### Попередження під час роботи

**encoder.status.magnet_high == true**
- Магніт занадто близько
- Збільшити відстань на 0.1-0.3 mm

**encoder.status.magnet_low == true**
- Магніт занадто далеко
- Зменшити відстань на 0.1-0.3 mm

**encoder.velocity дуже великі значення**
- Можливо, неправильна обробка переходу 0°/360°
- Перевірити алгоритм в `AEAT9922_ReadAngle()`

---

## Додаток C: Корисні посилання

### Документація:
- **Datasheet**: `ServoLib/Doc/AEAT-9922/AEAT-9922-DS105.pdf`
- **Application Note**: `ServoLib/Doc/AEAT-9922/AEAT-9922-Q24-AN101.pdf`
- **Reliability Data**: `ServoLib/Doc/AEAT-9922/AEAT-9922-RDS100.pdf`

### Посилання:
- [Broadcom AEAT-9922 Product Page](https://www.broadcom.com/products/motion-control-encoders/magnetic-encoders/aeat-9922)
- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00031020.pdf)
- [SPI Protocol Tutorial](https://www.analog.com/en/analog-dialogue/articles/introduction-to-spi-interface.html)

---

**Дата останнього оновлення:** 2025
**Версія документу:** 1.0
**Автори:** ServoCore Team, Дипломний проект КПІ
**Ліцензія:** MIT License

---

**Готово до імплементації!** 🚀
