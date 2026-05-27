# ServoLib - Технічна специфікація

## Версія

**Версія:** 0.2.0
**Дата:** 2026-05
**Платформи:** STM32F411CEU6 (BlackPill OCM3), STM32F411CEU6 (EncoderHub)
**HAL:** libopencm3 (не STM32 HAL)

---

## 1. Загальний огляд

ServoLib - це модульна бібліотека керування DC сервоприводами для embedded-систем на базі STM32. Бібліотека побудована на принципах шарової архітектури з повною абстракцією від апаратного забезпечення, що забезпечує портативність між різними платформами STM32.

### 1.1 Основні характеристики

- **Шарова архітектура** з чіткою ізоляцією логіки від апаратної частини
- **Hardware Driver Layer (HWD)** для абстракції мікроконтролера
- **Підтримка DC PWM двигунів** (одноканальний PWM+DIR, двоканальний H-bridge)
- **Каскадний PID** (pos→vel→trq) з feedforward та slew rate
- **Інтегрована система безпеки** з захистами та обмеженнями
- **Генератор траєкторій** з плавними переходами
- **Fail-safe електронні гальма** з автоматичним керуванням
- **Підтримка датчиків положення:** інкрементальний EXTI X4 + IC timer, AS5600 12-bit I2C
- **UKF фільтр** [θ, ω, α] для інкрементального енкодера (encoder_ukf)
- **Async UART комунікація** (COBS + CRC32 + packet_codec)
- **EncoderHub:** 6-канальний концентратор енкодерів з передачею по SPI

### 1.2 Область застосування

- Робототехніка (маніпулятори, рухомі платформи)
- Промислові системи позиціювання
- CNC машини
- 3D принтери
- Автоматизація виробництва

---

## 2. Архітектура системи

### 2.1 Шарова модель

```
┌─────────────────────────────────────────────────┐
│         Application Layer (Користувач)          │
├─────────────────────────────────────────────────┤
│   Control Layer (ctrl/)                         │
│   - Servo Controller                            │
│   - PID регулятори                              │
│   - Система безпеки                             │
│   - Генератор траєкторій                        │
│   - Калібрування                                │
├─────────────────────────────────────────────────┤
│   Driver Layer (drv/) з Hardware Callbacks      │
│   ┌───────────────────────────────────────────┐ │
│   │ Базова логіка:                            │ │
│   │ - Motor Interface (motor.c)               │ │
│   │ - Position Sensor Interface (position.c)  │ │
│   │ - Brake Interface (brake.c)               │ │
│   └────────────┬──────────────────────────────┘ │
│                │ викликає callbacks              │
│                ↓                                 │
│   ┌───────────────────────────────────────────┐ │
│   │ Hardware Callbacks:                       │ │
│   │ - PWM Motor (pwm.c)                       │ │
│   │ - Incremental Encoder (incremental_encoder.c) │ │
│   │ - AS5600 Sensor (as5600.c)                │ │
│   │ - GPIO Brake (gpio_brake.c)               │ │
│   └────────────┬──────────────────────────────┘ │
├────────────────┼─────────────────────────────────┤
│   Hardware Driver Layer (hwd/)                  │
│   - HWD_PWM, HWD_I2C, HWD_SPI                   │
│   - HWD_GPIO, HWD_Timer                         │
├─────────────────────────────────────────────────┤
│   Platform Layer (MCU/STM32F411_libopencm3/)    │
│   - Реалізація HWD через libopencm3             │
└─────────────────────────────────────────────────┘
```

### 2.2 Принципи дизайну

1. **Hardware Callbacks Pattern**: розділення базової логіки та апаратних операцій через callbacks
2. **Розділення відповідальності**: кожен шар має чітко визначені обов'язки
3. **Залежність від абстракцій**: логіка залежить від HWD, а не від HAL
4. **Універсальні інтерфейси**: motor, position, brake - працюють з будь-якою апаратурою
5. **Відкритість для розширення**: легко додати нові драйвери (просто реалізувати callbacks)
6. **Закритість для модифікації**: зміна платформи не впливає на логіку

---

## 3. Функціональні вимоги

### 3.1 Керування двигуном

#### FR-M-001: PWM керування
- Система ПОВИННА підтримувати PWM керування DC двигунами
- Частота PWM: 20 kHz (MOTOR_PWM_FREQ у board_config.h)
- Роздільна здатність: 1000 кроків (0.1%)
- Режими: одноканальний (PWM + DIR), двоканальний (PWM_FWD, PWM_BWD)

#### FR-M-002: Напрямок обертання
- Система ПОВИННА підтримувати:
  - Прямий хід
  - Зворотний хід
  - Інверсію напрямку (програмна)

#### FR-M-003: Статистика роботи
- Час роботи (мс)
- Кількість запусків
- Поточна потужність (%)
- Напрямок обертання
- Лічильник помилок

### 3.2 Зчитування датчиків

#### FR-S-001: Датчики положення
- **Incremental Encoder** (квадратурний, EXTI X4 + IC timer) — активний
  - Підтримка до 6 датчиків одночасно (ENC_MAX = 6)
  - 32-bit необмежений лічильник multi-turn у радіанах
  - IC-таймер вимірює `period_us` між фронтами → пряме обчислення ω без диференціювання
  - Валідація: відкидається вимір якщо `elapsed ≥ 65000 мкс` або перший імпульс
- **AS5600** (12-біт, I2C) — активний
  - Роздільна здатність: 4096 позицій на оберт
  - Частота оновлення: до 1 kHz (I2C IT continuous read у фоні)
- **AEAT-9922** — **видалено** з кодової бази

#### FR-S-002: UKF фільтр позиції (encoder_ukf)
- Стан: `[θ (рад), ω (рад/с), α (рад/с²)]`, вимірювання: `[θ]`
- Кінематична модель переходу: θ(k+1) = θ(k) + ω·dt + ½α·dt²
- `Encoder_UKF_Init`, `Encoder_UKF_Update(dt)`, `Encoder_UKF_GetState(θ, ω, α)`
- Окремий екземпляр `Encoder_UKF_t` для кожного датчика (static alloc)
- Тригер оновлення: 10 kHz (TIM3 на EncoderHub, control loop на OCM3)

#### FR-S-003: Обробка даних (Universal Position Interface)
- Multi-turn tracking (відстеження повних обертів всередині драйвера)
- `HW_ReadRaw()` → `angle_rad` завжди у радіанах
- `position.c` нормалізує до `[0, 2π)` і зберігає абсолютну позицію
- Всі публічні API повертають радіани; конвертація в градуси — лише в Apps

### 3.3 PID регулювання

#### FR-P-001: PID контролер
- Підтримка класичного PID алгоритму
- Коефіцієнти: Kp, Ki, Kd (налаштовувані)
- Anti-windup для інтегральної складової
- Обмеження виходу

#### FR-P-002: Режими роботи
- Позиційне керування (position control)
- Швидкісне керування (velocity control)
- Моментне керування (torque control) - опціонально

### 3.4 Система безпеки

#### FR-SAF-001: Обмеження
- Обмеження положення `min_position`/`max_position` (рад)
- Обмеження швидкості `max_velocity` (рад/с)
- Обмеження прискорення `max_acceleration` (рад/с²)
- Аварійний поріг струму `critical_current_a` (А)

#### FR-SAF-002: Захисти
- Захист від перевантаження по струму
- Захист від перегріву
- Watchdog таймер (таймаут керування)
- Детекція заклинювання двигуна

#### FR-SAF-003: Аварійна зупинка
- Миттєва зупинка двигуна
- Активація гальм (якщо є)
- Встановлення стану ERROR
- Запис коду помилки

### 3.5 Електронні гальма

#### FR-BR-001: Fail-safe логіка (Hardware Callbacks Pattern)
- Гальма АКТИВНІ за замовчуванням (ініціалізація у стані ENGAGED)
- State machine з transitions: ENGAGED → RELEASING → RELEASED → ENGAGING → ENGAGED
- Універсальний інтерфейс підтримує різні типи гальм (electromagnetic, pneumatic, hydraulic)
- GPIO driver для електромагнітних гальм (gpio_brake.c)

#### FR-BR-002: State Machine
- **ENGAGED** → `Brake_Release()` → **RELEASING** (затримка release_time_ms) → **RELEASED**
- **RELEASED** → `Brake_Engage()` → **ENGAGING** (затримка engage_time_ms) → **ENGAGED**
- Transitions обробляються автоматично в `Brake_Update()` кожен цикл

#### FR-BR-003: Параметри
- Затримка engage: 30-200 мс (типово 50 мс для electromagnetic)
- Затримка release: 30-200 мс (типово 30 мс для electromagnetic)
- Полярність сигналу: active_high (true/false) - налаштовується
- Emergency engage: миттєва активація без transition

### 3.6 Генератор траєкторій

#### FR-TR-001: Типи траєкторій
- Лінійна траєкторія
- S-крива (трапецеїдальний профіль швидкості)
- Параболічна траєкторія

#### FR-TR-002: Параметри руху
- Максимальна швидкість (рад/с)
- Максимальне прискорення (рад/с²)
- Максимальний ривок (рад/с³)
- Плавні переходи без стрибків

### 3.7 Калібрування

#### FR-CAL-001: Калібрування нуля
- Встановлення поточної позиції як нуля
- Збереження зміщення (offset)
- Автоматичне калібрування при старті

#### FR-CAL-002: Калібрування діапазону
- Визначення min/max положення
- Автоматична процедура калібрування
- Збереження калібровочних даних

---

## 4. Нефункціональні вимоги

### 4.1 Продуктивність

#### NFR-P-001: Частота оновлення
- Контур керування: 1000 Hz (1 мс період)
- Зчитування датчика: до 1000 Hz
- Оновлення PWM: до 100 kHz

#### NFR-P-002: Час відгуку
- Аварійна зупинка: < 10 мс
- Активація гальм: < 1 мс (програмно)
- Зміна потужності двигуна: < 1 мс

#### NFR-P-003: Використання пам'яті
- Статична пам'ять: < 10 KB
- Стек для однієї осі: < 1 KB
- Без динамічної алокації (no malloc)

### 4.2 Надійність

#### NFR-R-001: MTBF (Mean Time Between Failures)
- Цільовий показник: > 10000 годин
- Robustness до перехідних процесів живлення

#### NFR-R-002: Відновлення після помилок
- Автоматичне відновлення після non-critical помилок
- Логування всіх помилок
- Watchdog для детекції зависань

#### NFR-R-003: Валідація даних
- Перевірка всіх вказівників на NULL
- Перевірка діапазонів параметрів
- Перевірка статусів виконання функцій

### 4.3 Портативність

#### NFR-PORT-001: Незалежність від платформи
- Вся логіка (ctrl/, iface/, drv/) НЕ залежить від MCU
- Зміна в Board/ - і бібліотека працює на іншому STM32
- Підтримка різних STM32F4 (F411, F407, F446 тощо)

#### NFR-PORT-002: Незалежність від HAL
- Платформна реалізація — **libopencm3** (не STM32 HAL)
- `MCU/STM32F411_libopencm3/` — єдине місце з MCU-залежністю
- `ctrl/`, `drv/`, `comm/` не мають жодних `#include <libopencm3/...>`

### 4.4 Зручність використання

#### NFR-U-001: API
- Зрозумілі та послідовні імена функцій
- Уніфіковані коди помилок (Servo_Status_t)
- Документовані всі публічні функції (Doxygen)

#### NFR-U-002: Приклади
- Debug apps для кожного підсистеми (`Apps/debug_*`)
- Повний приклад у `servo_basic` (каскадний PID + async UART)

#### NFR-U-003: Налагодження
- Підтримка DEBUG режиму
- Логування помилок
- Опціональна статистика продуктивності

---

## 5. Технічні параметри

### 5.1 Апаратні вимоги

#### Мікроконтролер
- **MCU:** STM32F411CEU6 (основна платформа)
- **Тактова частота:** 100 MHz
- **Flash:** мін. 128 KB
- **RAM:** мін. 32 KB
- **Периферія:** TIM (x2), I2C (x1), GPIO

#### Двигун
- **Тип:** DC мотор з редуктором
- **Напруга:** 6-24V
- **Струм:** до 2A (типово)
- **Encoder:** опціонально

#### Датчик положення
- **Incremental Encoder:** квадратурний (EXTI X4), до 6 шт.
- **AS5600:** 12-біт магнітний енкодер
  - **Інтерфейс:** I2C (адреса 0x36)
  - **Роздільна здатність:** 4096 позицій/оберт
  - **Напруга живлення:** 3.3V або 5V

#### Електронні гальма
- **Тип:** Електромагнітні fail-safe
- **Напруга:** 12V або 24V
- **Струм:** до 500 mA
- **Керування:** GPIO (через драйвер реле/MOSFET)

### 5.2 Програмні вимоги

#### Середовище розробки
- **Build system:** CMake 3.16+
- **Toolchain:** `arm-none-eabi-gcc`
- **Standard:** C99
- **Платформна бібліотека:** libopencm3

#### Залежності
- libopencm3 (`LIBOPENCM3_DIR` змінна середовища)
- ukf_mcu (git submodule, для encoder_ukf та current UKF)
- frame_codec / packet_codec (git submodules, для async comm)
- Стандартна бібліотека C (без malloc для embedded)

---

## 6. Обмеження та припущення

### 6.1 Обмеження

1. **Одночасна робота максимум 6 осей** (MAX_AXES = 6)
2. **Частота PWM обмежена 100 kHz** (апаратне обмеження TIM)
3. **I2C швидкість до 400 kHz** (Fast Mode)
4. **Точність позиціювання ±0.5°** (залежить від механіки)

### 6.2 Припущення

1. **`Board_Init()` виконана** перед викликом ServoLib функцій
2. **Системний таймер налаштований** (SysTick для мс, TIM2 32-bit для мкс)
3. **Периферія сконфігурована в `board.c`** (TIM, I2C, GPIO, EXTI)
4. **Електроживлення стабільне** (без провалів напруги)
5. **Механічна частина справна** (без люфтів, без заклинювання)

---

## 7. Інтерфейси

### 7.1 Зовнішні інтерфейси

#### 7.1.1 Апаратні інтерфейси

**PWM виходи (TIM3):**
- PA6 (TIM3_CH1) - Прямий хід двигуна
- PA7 (TIM3_CH2) - Зворотний хід двигуна

**I2C шина (I2C1):**
- PB6 (I2C1_SCL) - Тактовий сигнал
- PB7 (I2C1_SDA) - Дані

**GPIO:**
- PA8 - Керування гальмами

**Таймер для мікросекунд (TIM2, 32-bit):**
- Free-running 1 MHz (prescaler 99 при 100 MHz)
- На EncoderHub: одночасно IC CH1-CH4 для ENC0-3

#### 7.1.2 Програмні інтерфейси

**Основні включення:**
```c
#include "ctrl/servo.h"              // Головний контролер
#include "drv/motor/motor.h"         // Universal motor interface
#include "drv/motor/pwm.h"           // PWM motor driver
#include "drv/position/position.h"   // Universal position interface
#include "drv/position/incremental_encoder.h"  // або as5600.h
#include "drv/brake/brake.h"         // Universal brake interface
#include "drv/brake/gpio_brake.h"    // GPIO brake driver
```

**Типові функції API:**
```c
// Ініціалізація
Servo_Status_t Servo_Init(Servo_Controller_t* servo, ...);

// Керування
Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position);
Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity);
Servo_Status_t Servo_Stop(Servo_Controller_t* servo);
Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo);

// Оновлення (викликати періодично)
Servo_Status_t Servo_Update(Servo_Controller_t* servo);

// Читання стану
float Servo_GetPosition(const Servo_Controller_t* servo);
float Servo_GetVelocity(const Servo_Controller_t* servo);
Servo_State_t Servo_GetState(const Servo_Controller_t* servo);
```

### 7.2 Внутрішні інтерфейси (Hardware Callbacks Pattern)

#### Motor_Interface_t
Універсальний інтерфейс двигуна з hardware callbacks.

```c
typedef struct {
    Motor_Data_t data;                     // Базова логіка (state, power, stats)
    Motor_Hardware_Callbacks_t hw;         // Hardware callbacks
    void* driver_data;                     // Вказівник на конкретний драйвер
} Motor_Interface_t;

// Hardware callbacks
typedef struct {
    Servo_Status_t (*init)(void* driver_data);
    Servo_Status_t (*set_power)(void* driver_data, const Motor_Command_t* cmd);
    Servo_Status_t (*stop)(void* driver_data);
    Servo_Status_t (*update)(void* driver_data);
} Motor_Hardware_Callbacks_t;
```

#### Position_Sensor_Interface_t
Універсальний інтерфейс датчика положення з hardware callbacks.

```c
typedef struct {
    Position_Sensor_Data_t data;           // Базова логіка (position, velocity, multi-turn)
    Position_Sensor_HW_Callbacks_t hw;     // Hardware callbacks
    void* driver_data;                     // Вказівник на конкретний драйвер
} Position_Sensor_Interface_t;

// Hardware callbacks
typedef struct {
    Servo_Status_t (*init)(void* driver_data);
    Servo_Status_t (*read_raw)(void* driver_data, Position_Raw_Data_t* raw_data);
    Servo_Status_t (*calibrate)(void* driver_data);
} Position_Sensor_HW_Callbacks_t;
```

#### Brake_Interface_t
Універсальний інтерфейс гальм з hardware callbacks.

```c
typedef struct {
    Brake_Data_t data;                     // Базова логіка (state machine, timing)
    Brake_Hardware_Callbacks_t hw;         // Hardware callbacks
    void* driver_data;                     // Вказівник на конкретний драйвер
} Brake_Interface_t;

// Hardware callbacks
typedef struct {
    Servo_Status_t (*init)(void* driver_data);
    Servo_Status_t (*engage)(void* driver_data);
    Servo_Status_t (*release)(void* driver_data);
    Servo_Status_t (*deinit)(void* driver_data);
} Brake_Hardware_Callbacks_t;
```

---

## 8. Безпека та захист

### 8.1 Рівні захисту

#### Рівень 1: Програмні обмеження
- Обмеження потужності (max_power)
- Обмеження швидкості (max_velocity)
- Обмеження прискорення (max_acceleration)

#### Рівень 2: Апаратні датчики
- Моніторинг струму (ADC)
- Моніторинг температури (опціонально)
- Кінцеві вимикачі (опціонально)

#### Рівень 3: Fail-safe механізми
- Електронні гальма (завжди активні за замовчуванням)
- Watchdog таймер (аварійна зупинка при зависанні)
- Аварійна кнопка (апаратна лінія ESTOP)

### 8.2 Коди помилок

```c
typedef enum {
    ERR_NONE              = 0x0000,  // Немає помилок
    ERR_MOTOR_OVERCURRENT = 0x0001,  // Перевантаження по струму
    ERR_MOTOR_OVERHEAT    = 0x0002,  // Перегрів двигуна
    ERR_MOTOR_STALL       = 0x0003,  // Двигун заклинило
    ERR_SENSOR_LOST       = 0x0010,  // Втрата зв'язку з датчиком
    ERR_SENSOR_INVALID    = 0x0011,  // Некоректні дані датчика
    ERR_POSITION_LIMIT    = 0x0020,  // Вихід за межі положення
    ERR_VELOCITY_LIMIT    = 0x0021,  // Перевищення швидкості
    ERR_WATCHDOG          = 0x0030,  // Watchdog таймаут
    ERR_INIT_FAILED       = 0x0040   // Помилка ініціалізації
} Servo_Error_t;
```

---

## 9. Тестування

### 9.1 Unit тести
- Тестування окремих модулів (PID, траєкторії, математика)
- Mock-об'єкти для HWD шару
- Покриття коду > 80%

### 9.2 Інтеграційні тести
- Тестування взаємодії драйверів
- Тестування на реальному залізі (STM32F411)
- Стрес-тести (тривала робота, багато циклів)

### 9.3 Системні тести
- Позиціювання (точність, час відгуку)
- Аварійна зупинка (час реакції)
- Fail-safe гальма (активація при збої живлення)

---

## 10. Подальший розвиток

### 10.1 Версія 0.2.0 (Планується)
- [ ] Підтримка степпер-моторів
- [ ] BLDC драйвер
- [ ] Підтримка оптичних енкодерів
- [ ] Розширена система логування

### 10.2 Версія 0.3.0 (Майбутнє)
- [ ] Підтримка RTOS (FreeRTOS)
- [ ] CAN інтерфейс для багатоосьових систем
- [ ] GUI для налаштування (через UART/USB)
- [ ] Збереження налаштувань в EEPROM

### 10.3 Портування на інші платформи
- [ ] STM32F407
- [ ] STM32F446
- [ ] STM32H7
- [ ] ESP32 (перспектива)

---

## 11. Посилання та ресурси

### 11.1 Документація
- [../README.md](../README.md) - Швидкий старт та огляд
- [structure.md](structure.md) - Детальна структура проекту
- [BRAKE_DRIVER.md](BRAKE_DRIVER.md) - Документація драйвера гальм
- [tmp/encoder_hub_pinout.md](tmp/encoder_hub_pinout.md) — Pinout EncoderHub (STM32F411CEU6 UFQFPN48)
- [../CLAUDE.md](../CLAUDE.md) - Інструкції для Claude Code

### 11.2 Датшити
- STM32F411CEU6 Reference Manual
- AS5600 Magnetic Encoder Datasheet
- L298N H-Bridge Driver Datasheet

### 11.3 Стандарти
- MISRA C:2012 - Правила написання коду для embedded
- IEC 61508 - Functional Safety (для критичних систем)

---

## Додаток А: Глосарій

| Термін | Опис |
|--------|------|
| **HWD** | Hardware Driver Layer — шар абстракції апаратного забезпечення (Inc/hwd/) |
| **libopencm3** | Відкрита бібліотека для ARM Cortex-M; заміна STM32 HAL у цьому проекті |
| **PWM** | Pulse Width Modulation - широтно-імпульсна модуляція |
| **PID** | Proportional-Integral-Derivative controller - ПІД регулятор |
| **Fail-safe** | Режим безпечної відмови (гальма активні за замовчуванням) |
| **Anti-windup** | Обмеження інтегральної складової PID |
| **Trajectory** | Траєкторія - плавний профіль руху від точки A до точки B |
| **Watchdog** | Сторожовий таймер для детекції зависання програми |

---

## Додаток Б: Збірка та конфігурація

### Збірка

```bash
export LIBOPENCM3_DIR=/path/to/libopencm3
./configure.sh    # інтерактивний вибір BOARD / APP / PROGRAMMER
./build.sh        # cmake --build
./flash.sh        # openocd flash
```

`.preset` — sourceable bash файл зі станом: `BOARD`, `APP`, `PROGRAMMER`, `PROGRAMMER_SERIAL`.

### Цілі (Apps/)

| Ціль | Призначення |
|------|-------------|
| `debug_encoder` | Тест інкрементального енкодера + UKF |
| `debug_motor` | Тест PWM двигуна |
| `debug_brake` | Тест GPIO гальма |
| `debug_current` | Тест ACS712 датчика струму |
| `servo_full` | Повний сервопривід (Servo_Controller_t + Safety) |
| `servo_basic` | Каскадний PID + async UART DMA comm |

### Налаштування TIM2 (мікросекунди + IC для EncoderHub)

- **Prescaler:** 99 → 1 MHz при 100 MHz тактуванні
- **Counter Period:** 0xFFFFFFFF (32-bit, ~4294 с до переповнення)
- **OCM3 board:** тільки free-running `HWD_Timer_GetMicros()`
- **EncoderHub:** додатково IC CH1-4 для ENC0-3 (ідентичний prescaler)

### Налаштування TIM3 (PWM мотора, OCM3)

- **Prescaler:** 4 → 20 MHz → ARR=999 → 20 kHz
- **Channel 1 (PA6):** PWM Generation CH1 AF2

---

**Дата останнього оновлення:** 2026-05
**Версія документу:** 2.0
