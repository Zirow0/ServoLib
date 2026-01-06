# Драйвер електронних гальм - Документація

## Огляд

Драйвер електронних гальм забезпечує безпечне керування механічними гальмами сервоприводу з використанням **Hardware Callbacks Pattern**. Система має універсальний інтерфейс `Brake_Interface_t` з базовою логікою (state machine) та апаратно-специфічними callbacks.

**Ключові особливості:**
- **Універсальний інтерфейс** - підтримує різні типи гальм (electromagnetic, pneumatic, hydraulic)
- **State machine з transitions** - плавні переходи між станами (ENGAGED/RELEASING/RELEASED/ENGAGING)
- **Fail-safe логіка** - гальма активні за замовчуванням (без живлення)
- **GPIO driver** - готова реалізація для електромагнітних гальм

## Принцип роботи

### State Machine Architecture

Нова архітектура використовує state machine з чотирма станами та плавними transitions:

```
┌─────────────────────────────────────────────────────┐
│              State Machine Diagram                  │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Початковий стан: ENGAGED (гальма активні)          │
│                                                     │
│  Brake_Release() → RELEASING (transition)           │
│                    ↓ (release_time_ms)              │
│                    RELEASED (гальма відпущені)      │
│                                                     │
│  Brake_Engage() →  ENGAGING (transition)            │
│                    ↓ (engage_time_ms)               │
│                    ENGAGED (гальма активні)         │
│                                                     │
│  Brake_EmergencyEngage() → ENGAGED (миттєво)        │
│                                                     │
└─────────────────────────────────────────────────────┘
```

**Переваги state machine:**
- Плавні переходи для електромагнітних гальм (потрібен час на спрацювання)
- Можливість відстежувати, чи гальма ще переходять у стан
- Захист від передчасного руху (motion blocked while ENGAGING/ENGAGED)

### Часова діаграма з Transitions

```
Команди:      Release()         Engage()         Release()
                  ↓                 ↓                 ↓
Стан:     ENGAGED→RELEASING→RELEASED→ENGAGING→ENGAGED→RELEASING→RELEASED
                  |         |         |         |         |         |
Час:              |←30 мс→|         |←50 мс→|         |←30 мс→|

Рух:      ████████▓▓▓▓▓▓▓▓▓░░░░░░░░░▓▓▓▓▓▓▓▓▓▓████████▓▓▓▓▓▓▓▓░░░░░░░

████ - ENGAGED (гальма активні)
▓▓▓▓ - RELEASING/ENGAGING (transition)
░░░░ - RELEASED (гальма відпущені, рух дозволено)
```

**Важливо:** Під час transition (RELEASING/ENGAGING) рух заблокований. Мотор може обертатися тільки у стані RELEASED.

## Структура файлів (після рефакторингу)

```
ServoLib/
├── Inc/drv/brake/
│   ├── brake.h                # Універсальний інтерфейс з hardware callbacks
│   └── gpio_brake.h           # GPIO драйвер для electromagnetic brakes
└── Src/drv/brake/
    ├── brake.c                # Базова логіка (state machine, transitions)
    └── gpio_brake.c           # Hardware callbacks (GPIO operations)
```

**Архітектура:**
- **brake.c** - базова логіка, state machine, обробка transitions
- **gpio_brake.c** - апаратні операції (GPIO control для electromagnetic brakes)
- Інтерфейс **iface/brake** видалено - замінено на universal interface pattern

## API (Hardware Callbacks Pattern)

### Використання

**Крок 1: Включити заголовки**
```c
#include "drv/brake/brake.h"         // Універсальний інтерфейс
#include "drv/brake/gpio_brake.h"    // GPIO драйвер
```

**Крок 2: Створити driver з callbacks**
```c
// GPIO драйвер автоматично налаштовує hardware callbacks
GPIO_Brake_Driver_t brake_driver;
GPIO_Brake_Config_t config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,          // Active LOW для fail-safe
    .engage_time_ms = 50,          // 50 мс на спрацювання
    .release_time_ms = 30          // 30 мс на відпускання
};

GPIO_Brake_Create(&brake_driver, &config);
```

**Крок 3: Використовувати універсальний інтерфейс**
```c
// Доступ до інтерфейсу через &driver->interface
Brake_Release(&brake_driver.interface);
Brake_Engage(&brake_driver.interface);
Brake_Update(&brake_driver.interface);  // Викликати регулярно!
```

### Структури даних

#### Brake_State_t
```c
typedef enum {
    BRAKE_STATE_ENGAGED = 0,     // Гальма активні (стабільний стан)
    BRAKE_STATE_RELEASING = 1,   // Перехід до відпущення
    BRAKE_STATE_RELEASED = 2,    // Гальма відпущені (стабільний стан)
    BRAKE_STATE_ENGAGING = 3     // Перехід до активації
} Brake_State_t;
```

#### GPIO_Brake_Config_t
```c
typedef struct {
    void* gpio_port;            // GPIO порт (GPIOA, GPIOB, ...)
    uint16_t gpio_pin;          // GPIO пін (GPIO_PIN_0, ...)
    bool active_high;           // Полярність (true = HIGH активує, false = LOW)
    uint32_t engage_time_ms;    // Час переходу RELEASING → ENGAGED
    uint32_t release_time_ms;   // Час переходу ENGAGING → RELEASED
} GPIO_Brake_Config_t;
```

#### Brake_Interface_t (Universal Interface)
```c
typedef struct {
    Brake_Data_t data;                     // Базова логіка (state, timing)
    Brake_Hardware_Callbacks_t hw;         // Hardware callbacks
    void* driver_data;                     // Вказівник на GPIO_Brake_Driver_t
} Brake_Interface_t;
```

### Основні функції

#### GPIO_Brake_Create
Створення та ініціалізація GPIO brake driver з hardware callbacks.

```c
Servo_Status_t GPIO_Brake_Create(GPIO_Brake_Driver_t* driver,
                                  const GPIO_Brake_Config_t* config);
```

**Параметри:**
- `driver` - вказівник на GPIO brake driver
- `config` - конфігурація GPIO гальм

**Повертає:** `SERVO_OK` при успіху

**Що робить:**
1. Ініціалізує GPIO для керування гальмами
2. Налаштовує hardware callbacks (engage, release)
3. Викликає `Brake_Init()` для ініціалізації базової логіки
4. Встановлює початковий стан ENGAGED (гальма активні)

**Приклад:**
```c
GPIO_Brake_Driver_t brake_driver;

GPIO_Brake_Config_t config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,          // Active LOW (fail-safe)
    .engage_time_ms = 50,          // 50 мс на спрацювання
    .release_time_ms = 30          // 30 мс на відпускання
};

// Створює driver та ініціалізує GPIO + callbacks
GPIO_Brake_Create(&brake_driver, &config);
```

#### Brake_Release
Відпускання гальм (дозвіл руху).

```c
Servo_Status_t Brake_Release(Brake_Interface_t* brake);
```

**Поведінка:**
- Переключає state: ENGAGED → RELEASING
- Викликає `brake->hw.release()` callback (GPIO operation)
- Через `release_time_ms` автоматично переключає на RELEASED (в `Brake_Update()`)

**Приклад:**
```c
// Через universal interface
Brake_Release(&brake_driver.interface);

// Стан тепер RELEASING, через 30 мс → RELEASED
```

#### Brake_Engage
Активація гальм (блокування руху).

```c
Servo_Status_t Brake_Engage(Brake_Interface_t* brake);
```

**Поведінка:**
- Переключає state: RELEASED → ENGAGING
- Викликає `brake->hw.engage()` callback (GPIO operation)
- Через `engage_time_ms` автоматично переключає на ENGAGED (в `Brake_Update()`)

**Приклад:**
```c
Brake_Engage(&brake_driver.interface);

// Стан тепер ENGAGING, через 50 мс → ENGAGED
```

#### Brake_Update
Обробка transitions між станами (викликати періодично!).

```c
Servo_Status_t Brake_Update(Brake_Interface_t* brake);
```

**Що робить:**
- Перевіряє, чи минув час transition (engage_time_ms або release_time_ms)
- Переключає стан з RELEASING → RELEASED або ENGAGING → ENGAGED
- **КРИТИЧНО:** Без регулярних викликів transitions не відбуваються!

**Приклад:**
```c
// У головному циклі (1 kHz)
while (1) {
    Brake_Update(&brake_driver.interface);
    HAL_Delay(1);
}
```

#### Brake_EmergencyEngage
Аварійна активація гальм (миттєво, без transition).

```c
Servo_Status_t Brake_EmergencyEngage(Brake_Interface_t* brake);
```

**Поведінка:**
- Миттєво переключає на ENGAGED (без ENGAGING transition)
- Викликає `brake->hw.engage()` callback
- Використовується при критичних помилках (перевантаження, перегрів)

**Приклад:**
```c
// При аварійній ситуації
Brake_EmergencyEngage(&brake_driver.interface);  // Миттєво блокувати

// Стан ENGAGED без затримки!
```

#### Brake_GetState
Отримання поточного стану.

```c
Brake_State_t Brake_GetState(const Brake_Interface_t* brake);
```

**Повертає:** `BRAKE_STATE_ENGAGED`, `BRAKE_STATE_RELEASING`, `BRAKE_STATE_RELEASED`, або `BRAKE_STATE_ENGAGING`

**Приклад:**
```c
Brake_State_t state = Brake_GetState(&brake_driver.interface);
if (state == BRAKE_STATE_RELEASED) {
    // Можна рухатися
}
```

#### Brake_IsEngaged / Brake_IsReleased
Швидка перевірка стабільного стану.

```c
bool Brake_IsEngaged(const Brake_Interface_t* brake);
bool Brake_IsReleased(const Brake_Interface_t* brake);
bool Brake_IsTransitioning(const Brake_Interface_t* brake);
```

**Приклад:**
```c
if (Brake_IsReleased(&brake_driver.interface)) {
    // Гальма відпущені, можна рухатися
}

if (Brake_IsTransitioning(&brake_driver.interface)) {
    // Гальма у процесі переходу, зачекати
}
```

## Інтеграція з Servo_Controller

Драйвер гальм повністю інтегрований в контролер сервоприводу.

### Ініціалізація

```c
// 1. Створити GPIO brake driver
GPIO_Brake_Driver_t brake_driver;
GPIO_Brake_Config_t brake_config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,          // Active LOW (fail-safe)
    .engage_time_ms = 50,
    .release_time_ms = 30
};
GPIO_Brake_Create(&brake_driver, &brake_config);

// 2. Конфігурація сервоконтролера
Servo_Config_t servo_config = {
    .update_frequency = 1000.0f,
    .enable_brake = true           // Увімкнути гальма
    /* ... PID, safety, trajectory params ... */
};

// 3. Ініціалізація servo з motor та brake interfaces
Servo_InitWithBrake(&servo_controller, &servo_config,
                    &motor_driver.interface,
                    &brake_driver.interface);  // Universal interface!
```

### Автоматичне керування

Після інтеграції Servo контролер автоматично керує гальмами через state machine:

```c
// 1. При команді руху - Servo викличе Brake_Release()
Servo_SetPosition(&servo_controller, 90.0f);
// Стан гальм: ENGAGED → RELEASING → RELEASED (через 30 мс)

// 2. Servo_Update() автоматично викликає Brake_Update()
// (обробляє transitions між станами)

// 3. При аварійній зупинці - Servo викличе Brake_EmergencyEngage()
Servo_EmergencyStop(&servo_controller);
// Стан гальм: МИТТЄВО ENGAGED (без transition)
```

**Важливо:** Servo контролер автоматично:
- Відпускає гальма перед рухом (Brake_Release)
- Блокує гальма після завершення руху
- Викликає Brake_Update() у кожному Servo_Update()
- Використовує EmergencyEngage при критичних помилках

## Апаратне підключення

### Схема підключення

```
STM32F411          Brake Driver         Motor
──────────         ────────────         ──────
PA8 (GPIO)  ──────► Control Input
                    Power Output  ──────► Electromagnetic Brake
                    GND          ──────► Common GND
                    VCC          ◄────── 12V/24V Supply
```

### Типи гальм

#### 1. Fail-Safe Electromagnetic Brakes (рекомендовано)
```
active_high = false (Active LOW):
  GPIO HIGH → Струм НЕМАЄ → Гальма АКТИВНІ (пружина стискає)
  GPIO LOW  → Струм Є     → Гальма ВІДПУЩЕНІ (електромагніт тримає)

Переваги: При втраті живлення гальма автоматично активуються!
```

#### 2. Стандартні Electromagnetic Brakes
```
active_high = true (Active HIGH):
  GPIO HIGH → Струм Є     → Гальма АКТИВНІ (електромагніт притискує)
  GPIO LOW  → Струм НЕМАЄ → Гальма ВІДПУЩЕНІ (пружина розтискає)

Недоліки: При втраті живлення гальма відпущені (небезпечно!)
```

**Рекомендація:** Завжди використовуйте fail-safe (active_low) для безпеки!

### GPIO налаштування в CubeMX

**BRAKE_CTRL (PA8) для Fail-Safe гальм (active_high=false):**
- GPIO mode: Output Push Pull
- Pull-up/Pull-down: No pull (або Pull-up для додаткової безпеки)
- Output speed: Low
- **Initial state: HIGH** (гальма активні при старті!)
- User Label: `BRAKE_CTRL`

**Для стандартних гальм (active_high=true):**
- Все те саме, але Initial state: LOW

## Налаштування параметрів

### Рекомендовані значення

```c
GPIO_Brake_Config_t brake_config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,          // Fail-safe (рекомендовано!)

    // Для швидких електромагнітних гальм
    .engage_time_ms = 30,          // Швидке спрацювання
    .release_time_ms = 20,         // Швидке відпускання

    // Для стандартних електромагнітних гальм (рекомендовано)
    .engage_time_ms = 50,          // Середня швидкість
    .release_time_ms = 30,         // Середня швидкість

    // Для важких/великих гальм
    .engage_time_ms = 100,         // Повільне спрацювання
    .release_time_ms = 80,         // Повільне відпускання

    // Для пневматичних/гідравлічних гальм
    .engage_time_ms = 200,         // Дуже повільно
    .release_time_ms = 150         // Дуже повільно
};
```

### Підбір часу transitions

**engage_time_ms** (час на повне спрацювання):
- **20-30 мс** - малі швидкі гальма
- **30-50 мс** - стандартні електромагнітні гальма
- **50-100 мс** - великі важкі гальма
- **100-200 мс** - пневматичні/гідравлічні

**release_time_ms** (час на повне відпускання):
- Зазвичай на 30-40% швидше, ніж engage_time_ms
- Електромагнітні гальма відпускаються швидше за спрацювання

## Додавання нових типів гальм

Для додавання підтримки нових типів гальм (наприклад, pneumatic, hydraulic):

### 1. Створити новий driver

```c
// Inc/drv/brake/pneumatic_brake.h
typedef struct {
    Brake_Interface_t interface;  // Universal interface (ПЕРШЕ ПОЛЕ!)
    // Апаратно-специфічні поля (valve control, pressure sensor, etc.)
    void* valve_port;
    uint16_t valve_pin;
    float target_pressure;
} Pneumatic_Brake_Driver_t;
```

### 2. Реалізувати hardware callbacks

```c
// Src/drv/brake/pneumatic_brake.c
static Servo_Status_t Pneumatic_HW_Engage(void* driver_data) {
    Pneumatic_Brake_Driver_t* driver = (Pneumatic_Brake_Driver_t*)driver_data;
    // Відкрити клапан для подачі повітря
    // ...
    return SERVO_OK;
}

static Servo_Status_t Pneumatic_HW_Release(void* driver_data) {
    Pneumatic_Brake_Driver_t* driver = (Pneumatic_Brake_Driver_t*)driver_data;
    // Закрити клапан, випустити повітря
    // ...
    return SERVO_OK;
}
```

### 3. Створити factory function

```c
Servo_Status_t Pneumatic_Brake_Create(Pneumatic_Brake_Driver_t* driver,
                                       const Pneumatic_Brake_Config_t* config) {
    // Налаштувати апаратуру
    // ...

    // Налаштувати callbacks
    driver->interface.hw.init = Pneumatic_HW_Init;
    driver->interface.hw.engage = Pneumatic_HW_Engage;
    driver->interface.hw.release = Pneumatic_HW_Release;
    driver->interface.driver_data = driver;

    // Ініціалізувати базову логіку
    Brake_Params_t params = {
        .engage_time_ms = config->engage_time_ms,
        .release_time_ms = config->release_time_ms
    };
    return Brake_Init(&driver->interface, &params);
}
```

### 4. Використовувати

```c
Pneumatic_Brake_Driver_t pneumatic_brake;
Pneumatic_Brake_Config_t config = { /* ... */ };
Pneumatic_Brake_Create(&pneumatic_brake, &config);

// Використовувати через universal interface!
Brake_Release(&pneumatic_brake.interface);
Brake_Update(&pneumatic_brake.interface);
```

**Переваги:** Вся базова логіка (state machine, transitions) вже реалізована в `brake.c`. Потрібно лише реалізувати апаратні операції!

## Діагностика

### Перевірка стану

```c
// Отримати стан гальм
Brake_State_t state = Brake_GetState(&brake_driver.interface);

if (state == BRAKE_STATE_ENGAGED) {
    // Гальма повністю активні
} else if (state == BRAKE_STATE_RELEASING) {
    // Гальма у процесі відпускання
} else if (state == BRAKE_STATE_RELEASED) {
    // Гальма повністю відпущені, можна рухатися
} else if (state == BRAKE_STATE_ENGAGING) {
    // Гальма у процесі спрацювання
}

// Або використовувати helper functions
if (Brake_IsReleased(&brake_driver.interface)) {
    // Гальма відпущені, рух дозволено
}

if (Brake_IsTransitioning(&brake_driver.interface)) {
    // Гальма у процесі переходу, зачекати
}
```

### Типові проблеми

| Проблема | Можлива причина | Рішення |
|----------|----------------|---------|
| Гальма не відпускають | Неправильна полярність | Змінити `active_high` в конфігурації |
| Гальма не відпускають | `Brake_Update()` не викликається | Додати `Brake_Update()` у main loop |
| Двигун не обертається | Гальма не у стані RELEASED | Перевірити стан, зачекати transitions |
| Transitions занадто повільні | Великі значення часу | Зменшити `engage_time_ms` / `release_time_ms` |
| Transitions занадто швидкі | Малі значення часу | Збільшити час відповідно до апаратури |
| GPIO не працює | Неправильна ініціалізація | Перевірити CubeMX налаштування |

## Приклади використання

### Базове використання (без Servo)

```c
// 1. Ініціалізація
GPIO_Brake_Driver_t brake_driver;
GPIO_Brake_Config_t config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,          // Fail-safe
    .engage_time_ms = 50,
    .release_time_ms = 30
};
GPIO_Brake_Create(&brake_driver, &config);

// 2. Головний цикл
while (1) {
    // ОБОВ'ЯЗКОВО викликати Update для обробки transitions!
    Brake_Update(&brake_driver.interface);

    // Керування рухом
    if (need_to_move) {
        // Відпустити гальма
        Brake_Release(&brake_driver.interface);

        // Зачекати повного відпускання
        while (Brake_IsTransitioning(&brake_driver.interface)) {
            Brake_Update(&brake_driver.interface);
            HAL_Delay(1);
        }

        // Тепер можна рухатися
        Motor_SetPower(&motor, 50.0f);
    }

    HAL_Delay(1);
}
```

### З Servo_Controller (автоматичне керування)

```c
// Servo контролер автоматично керує гальмами!
Servo_SetPosition(&servo, 90.0f);
// Servo автоматично:
// 1. Викличе Brake_Release()
// 2. Зачекає RELEASED стану
// 3. Почне рух
// 4. Після завершення - Brake_Engage()
```

## Технічні характеристики

- **Архітектура**: Hardware Callbacks Pattern
- **State Machine**: 4 стани (ENGAGED, RELEASING, RELEASED, ENGAGING)
- **Час реакції**: < 1 мс (програмне переключення GPIO)
- **Transition timing**: Налаштовується (20-200 мс типово)
- **Роздільна здатність таймерів**: 1 мс
- **Підтримувані напруги**: 3.3V логіка GPIO (через драйвер для високої напруги)
- **Типи гальм**: Electromagnetic, Pneumatic, Hydraulic (через callbacks)
- **Fail-Safe**: Так (fail-safe brakes активні за замовчуванням при active_high=false)

## Безпека

### Критичні вимоги

✅ **Обов'язково:**
- Гальма ЗАВЖДИ активні при відсутності живлення (використовувати active_high=false)
- Використовувати fail-safe електромагнітні гальма
- Викликати `Brake_Update()` регулярно (мінімум 100 Hz, рекомендовано 1 kHz)
- Перевіряти стан (`Brake_IsReleased()`) перед рухом
- Зачекати завершення transitions перед рухом

❌ **Заборонено:**
- Блокувати виклики `Brake_Update()` (transitions не відбудуться!)
- Використовувати гальма без fail-safe в критичних застосуваннях
- Ігнорувати помилки ініціалізації
- Рухатися під час ENGAGING/RELEASING станів
- Використовувати active_high=true для важких навантажень (небезпечно!)

## Автор

ServoCore Team - Дипломний проект КПІ, 2025

## Ліцензія

MIT License

---

**Готово до використання!** 🔒

Повернутися до: [../README.md](../README.md) | [structure.md](structure.md) | [technical_specifications.md](technical_specifications.md)
