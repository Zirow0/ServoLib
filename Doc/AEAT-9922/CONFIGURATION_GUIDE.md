# AEAT-9922 Configuration Guide

## Огляд

AEAT-9922 підтримує **множинні режими роботи одночасно** через систему битових прапорців. Це дозволяє комбінувати різні інтерфейси для оптимальної роботи.

## Режими роботи (Mode Flags)

### Доступні режими

```c
typedef enum {
    AEAT9922_MODE_NONE    = 0,         // Жоден режим

    // Абсолютні інтерфейси
    AEAT9922_MODE_SPI3    = (1 << 0),  // SPI-3 (memory R/W)
    AEAT9922_MODE_SSI3    = (1 << 1),  // SSI-3 (2 або 3 wire)
    AEAT9922_MODE_SSI2    = (1 << 2),  // SSI-2 (2 wire)
    AEAT9922_MODE_SPI4    = (1 << 3),  // SPI-4 (діагностика + position)
    AEAT9922_MODE_PWM     = (1 << 4),  // PWM вихід

    // Інкрементальні виходи
    AEAT9922_MODE_ABI     = (1 << 5),  // ABI інкрементальний
    AEAT9922_MODE_UVW     = (1 << 6),  // UVW комутація
} AEAT9922_Mode_Flags_t;
```

### Комбінації режимів

**Рекомендована комбінація (SPI4 + ABI):**
```c
config.enabled_modes = AEAT9922_MODE_SPI4 | AEAT9922_MODE_ABI;
```

**Інші комбінації:**
```c
// SSI + ABI
config.enabled_modes = AEAT9922_MODE_SSI3 | AEAT9922_MODE_ABI;

// SPI4 + ABI + UVW (для BLDC моторів)
config.enabled_modes = AEAT9922_MODE_SPI4 | AEAT9922_MODE_ABI | AEAT9922_MODE_UVW;

// Тільки PWM вихід
config.enabled_modes = AEAT9922_MODE_PWM;
```

## Структура конфігурації

### 1. Загальні налаштування (General Config)

```c
typedef struct {
    AEAT9922_Abs_Resolution_t abs_resolution; // 10-18 біт
    bool direction_ccw;                       // Напрямок обертання
    bool auto_zero_on_init;                   // Калібрувати при старті
    bool enable_inl_correction;               // INL корекція
    float hysteresis_deg;                     // Гістерезис (0.02° рекомендовано)
} AEAT9922_General_Config_t;
```

**Приклад:**
```c
.general = {
    .abs_resolution = AEAT9922_ABS_RES_18BIT,  // 262144 позицій
    .direction_ccw = false,                     // CW
    .auto_zero_on_init = true,
    .enable_inl_correction = true,
    .hysteresis_deg = 0.02f
}
```

### 2. SPI конфігурація

```c
typedef struct {
    HWD_SPI_Config_t spi_config;              // Апаратна конфігурація
    void* msel_port;                           // GPIO порт для MSEL
    uint16_t msel_pin;                         // GPIO пін для MSEL
    AEAT9922_Protocol_Variant_t protocol_variant;  // 16-bit або 24-bit
} AEAT9922_SPI_Config_t;
```

**Варіанти протоколу (PSEL bits):**
- `AEAT9922_PSEL_SPI4_16BIT` - 16-bit з парністю (швидше)
- `AEAT9922_PSEL_SPI4_24BIT` - 24-bit з CRC (надійніше) ✅ Рекомендовано

**Приклад:**
```c
.spi_config = {
    .spi_config.spi_handle = &hspi1,
    .msel_port = GPIOA,
    .msel_pin = GPIO_PIN_4,
    .protocol_variant = AEAT9922_PSEL_SPI4_24BIT  // 24-bit CRC
}
```

### 3. ABI Incremental конфігурація

```c
typedef struct {
    uint16_t incremental_cpr;                 // CPR (1-10000)
    AEAT9922_Index_Width_t index_width;       // Ширина Index pulse
    AEAT9922_Index_State_t index_state;       // Позиція Index pulse
    bool enable_incremental;                  // Використовувати TIM
    void* encoder_timer_handle;               // TIM handle
} AEAT9922_ABI_Config_t;
```

**Index Width варіанти:**
- `AEAT9922_INDEX_WIDTH_90` - 90° електричних
- `AEAT9922_INDEX_WIDTH_180` - 180°
- `AEAT9922_INDEX_WIDTH_270` - 270°
- `AEAT9922_INDEX_WIDTH_360` - 360°

**Index State позиції:**
- `AEAT9922_INDEX_STATE_90` - Index на 90°
- `AEAT9922_INDEX_STATE_180` - Index на 180°
- `AEAT9922_INDEX_STATE_270` - Index на 270°
- `AEAT9922_INDEX_STATE_360` - Index на нульовій позиції ✅ Рекомендовано

**Приклад:**
```c
.abi = {
    .incremental_cpr = 4096,                    // 4096 імпульсів/оберт
    .index_width = AEAT9922_INDEX_WIDTH_90,
    .index_state = AEAT9922_INDEX_STATE_360,    // Index на zero
    .enable_incremental = true,
    .encoder_timer_handle = &htim2              // TIM2 в Encoder Mode
}
```

### 4. UVW Commutation конфігурація

```c
typedef struct {
    uint8_t pole_pairs;  // Кількість пар полюсів (1-32)
} AEAT9922_UVW_Config_t;
```

**Приклад для BLDC мотора:**
```c
.uvw = {
    .pole_pairs = 7  // 7 пар полюсів = 14 полюсів
}
```

## Повний приклад конфігурації

### SPI4-B (24-bit CRC) + ABI

```c
#include "drv/position/aeat9922.h"

AEAT9922_Driver_t encoder;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;

void Init_Encoder(void)
{
    // Конфігурація
    AEAT9922_Config_t config = {
        // Режими: SPI4 для діагностики + ABI для швидкості
        .enabled_modes = AEAT9922_MODE_SPI4 | AEAT9922_MODE_ABI,

        // Загальні налаштування
        .general = {
            .abs_resolution = AEAT9922_ABS_RES_18BIT,
            .direction_ccw = false,
            .auto_zero_on_init = true,
            .enable_inl_correction = true,
            .hysteresis_deg = 0.02f
        },

        // SPI4-B (24-bit з CRC)
        .spi_config = {
            .spi_config.spi_handle = &hspi1,
            .msel_port = GPIOA,
            .msel_pin = GPIO_PIN_4,
            .protocol_variant = AEAT9922_PSEL_SPI4_24BIT
        },

        // ABI інкрементальний
        .abi = {
            .incremental_cpr = 4096,
            .index_width = AEAT9922_INDEX_WIDTH_90,
            .index_state = AEAT9922_INDEX_STATE_360,
            .enable_incremental = true,
            .encoder_timer_handle = &htim2
        },

        // UVW не використовується
        .uvw = {
            .pole_pairs = 0
        }
    };

    // Створити драйвер
    AEAT9922_Create(&encoder, &config);

    // Ініціалізувати Position Sensor
    Position_Params_t params = {
        .type = SENSOR_TYPE_AEAT9922,
        .resolution_bits = 18,
        .min_angle = 0.0f,
        .max_angle = 360.0f,
        .update_rate = 1000
    };

    Position_Sensor_Init(&encoder.interface, &params);
}
```

## Використання в Control Loop

### Основний цикл (1 kHz)

```c
void Control_Loop(void)
{
    while (1) {
        // 1. Оновити інкрементальний лічильник з TIM2
        AEAT9922_UpdateIncrementalCount(&encoder);

        // 2. Оновити Position Sensor (SPI read, velocity calc)
        Position_Sensor_Update(&encoder.interface);

        // 3. Отримати дані
        float position, velocity;
        Position_Sensor_GetPosition(&encoder.interface, &position);
        Position_Sensor_GetVelocity(&encoder.interface, &velocity);

        // 4. PID регулювання
        float error = target - position;
        float output = PID_Calculate(error);

        // 5. Застосувати до мотора
        Motor_SetPower_DC(&motor, output);

        HAL_Delay(1);  // 1 ms
    }
}
```

### Періодична діагностика (100 ms)

```c
void Diagnostics_Task(void)
{
    // Читати статус через SPI4
    AEAT9922_ReadStatus(&encoder);

    // Перевірити прапорці
    if (encoder.status.magnet_high) {
        // Warning: магніт занадто близько
    }

    if (encoder.status.magnet_low) {
        // Warning: магніт занадто далеко
    }

    if (encoder.status.memory_error) {
        // Critical: помилка EEPROM
        Error_Handler();
    }

    // Перевірити SPI errors
    if (encoder.error_count > 0) {
        // Warning: SPI помилки
    }
}
```

## Константи з Datasheet

### Timing константи (через #define)

```c
// Power-Up
#define AEAT9922_POWERUP_TIME_MS            10

// SPI4 Timing
#define AEAT9922_SPI4_T_CSN_MIN_NS          350
#define AEAT9922_SPI4_T_CSR_MIN_NS          350
#define AEAT9922_SPI4_T_CLK_MIN_NS          100

// ABI Timing
#define AEAT9922_ABI_REACTION_TIME_MS       10
#define AEAT9922_ABI_MAX_FREQUENCY_HZ       1000000

// CPR Limits
#define AEAT9922_INC_CPR_MIN                1
#define AEAT9922_INC_CPR_MAX                10000
```

## Апаратне налаштування

### SPI1 (CubeMX)

```
Mode: Full-Duplex Master
Data Size: 8 Bits
Prescaler: 16 (для 5.25 MHz при APB2=84MHz)
CPOL: Low (0)
CPHA: 2 Edge (1)
NSS: Software

Pins:
- PA5: SPI1_SCK  → M2 (AEAT-9922 pin 11)
- PA6: SPI1_MISO → M3 (AEAT-9922 pin 12)
- PA7: SPI1_MOSI → M1 (AEAT-9922 pin 10)
- PA4: CS (GPIO) → M0 (AEAT-9922 pin 9)
- PA4: MSEL (GPIO) → MSEL (AEAT-9922 pin 19)
```

### TIM2 Encoder Mode (CubeMX)

```
Mode: Encoder Mode TI1 and TI2
Encoder Mode: Encoder Mode 3 (обидва edges)
Counter Period: 65535 (16-bit) або 4294967295 (32-bit)

Pins:
- PA15: TIM2_CH1 → A (AEAT-9922 pin 24)
- PB3:  TIM2_CH2 → B (AEAT-9922 pin 23)

Опціонально Index:
- PA10: GPIO Input (EXTI) → I (AEAT-9922 pin 20)
```

## Поширені помилки

### 1. Неправильний MSEL

❌ **Помилка:**
```c
// MSEL=0, але enabled_modes має SPI4
.enabled_modes = AEAT9922_MODE_SPI4
```

✅ **Правильно:**
Драйвер автоматично встановлює MSEL=1 для SPI4/PWM/UVW

### 2. Неправильний PSEL

❌ **Помилка:**
```c
// Використовувати 16-bit для критичних застосувань
.protocol_variant = AEAT9922_PSEL_SPI4_16BIT
```

✅ **Правильно:**
```c
// 24-bit з CRC для надійності
.protocol_variant = AEAT9922_PSEL_SPI4_24BIT
```

### 3. Забули оновлювати incremental count

❌ **Помилка:**
```c
void Loop(void) {
    Position_Sensor_Update(&encoder.interface);  // Немає UpdateIncrementalCount!
}
```

✅ **Правильно:**
```c
void Loop(void) {
    AEAT9922_UpdateIncrementalCount(&encoder);  // Оновити TIM2
    Position_Sensor_Update(&encoder.interface);
}
```

## Переваги нового підходу

### 1. Гнучкість режимів
- ✅ Одночасно SPI4 (діагностика) + ABI (швидкість)
- ✅ Легко додавати нові режими (UVW, PWM)
- ✅ Битові маски для ефективності

### 2. Чистота конфігурації
- ✅ Всі timing константи через #define (з datasheet)
- ✅ Немає зайвих параметрів (voltage, magnet_type)
- ✅ PSEL через enum (не use_crc)

### 3. Масштабованість
- ✅ Легко додати SSI2/SSI3/PWM режими
- ✅ Підтримка UVW для BLDC
- ✅ Архітектура готова до розширення

## Посилання

- [AEAT-9922 Datasheet](AEAT-9922-DS105.pdf)
- [Приклад SPI4+ABI](../../Examples/aeat9922_spi4_abi_example.c)
- [Quick Start](../../Examples/aeat9922_quick_start.c)
