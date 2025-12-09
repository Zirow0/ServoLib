# Драйвер електронних гальм - Документація

## Огляд

Драйвер електронних гальм забезпечує безпечне керування механічними гальмами сервоприводу з використанням fail-safe логіки. Гальма активні за замовчуванням і автоматично блокують двигун при відсутності активності або живлення.

## Принцип роботи

### Fail-Safe логіка

```
┌─────────────────────────────────────────────────────┐
│                 Стан системи                        │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Початковий стан  →  ГАЛЬМА АКТИВНІ (блокування)   │
│                                                     │
│  Команда руху     →  Затримка 100 мс                │
│                      ↓                              │
│                      ГАЛЬМА ВІДПУЩЕНІ (рух)         │
│                                                     │
│  Рух виконано     →  Очікування 3 сек               │
│                                                     │
│  Без активності   →  ГАЛЬМА АКТИВНІ (блокування)    │
│                                                     │
│  Аварійна зупинка →  НЕГАЙНО ГАЛЬМА АКТИВНІ         │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Часова діаграма

```
Команди руху:     ↓               ↓
                  |               |
Активність:   ────┬───────────────┬──────────────
              |   |   Рух 1   |   |   Рух 2   |
              ↓   ↓           ↓   ↓           ↓
Гальма:   ████████░░░░░░░░░░░░████░░░░░░░░░░░░████
          ↑       ↑           ↑   ↑           ↑
       Блок.  Відпущ.      Блок. Відп.     Блок.
          |       |←──────→|   |←────→|
          |       |         |   |      |
          |    100мс    3 сек  100мс  3 сек
          |   затримка  тайм. затр.  тайм.

████ - Гальма активні (блокування)
░░░░ - Гальма відпущені (рух дозволено)
```

## Структура файлів

```
ServoLib/
├── Inc/
│   ├── drv/brake/
│   │   └── brake.h            # Драйвер гальм (низькорівневий)
│   └── iface/
│       └── brake.h            # Інтерфейс взаємодії (високорівневий)
└── Src/
    ├── drv/brake/
    │   └── brake.c            # Реалізація драйвера
    └── iface/
        └── brake.c            # Реалізація інтерфейсу
```

## API

### Вибір рівня API

**Високорівневий API (Рекомендовано):**
```c
#include "iface/brake.h"

// Простий API для базового використання
BrakeInterface_Init(&params);
BrakeInterface_Release();
BrakeInterface_Engage();
```

**Низькорівневий API (Розширений):**
```c
#include "drv/brake/brake.h"

// Прямий доступ до драйвера для складних сценаріїв
Brake_Init(&brake_driver, &config);
Brake_Release(&brake_driver);
```

### Структури даних

#### Brake_State_t
```c
typedef enum {
    BRAKE_STATE_ENGAGED = 0,    // Гальма активні
    BRAKE_STATE_RELEASED = 1    // Гальма відпущені
} Brake_State_t;
```

#### Brake_Mode_t
```c
typedef enum {
    BRAKE_MODE_MANUAL,          // Ручне керування
    BRAKE_MODE_AUTO             // Автоматичне керування
} Brake_Mode_t;
```

#### Brake_Config_t
```c
typedef struct {
    HWD_GPIO_Handle_t* gpio;    // GPIO дескриптор
    uint16_t pin;               // Номер піна
    bool active_high;           // Полярність сигналу
    uint32_t release_delay_ms;  // Затримка відпускання (мс)
    uint32_t engage_timeout_ms; // Таймаут блокування (мс)
} Brake_Config_t;
```

### Основні функції

#### Brake_Init
Ініціалізація драйвера гальм.

```c
Servo_Status_t Brake_Init(Brake_Driver_t* brake,
                           const Brake_Config_t* config);
```

**Параметри:**
- `brake` - вказівник на драйвер
- `config` - конфігурація гальм

**Повертає:** `SERVO_OK` при успіху

**Приклад:**
```c
// Конфігурація GPIO
HWD_GPIO_Config_t gpio_config = {
    .port = BRAKE_CTRL_GPIO_Port,
    .mode = HWD_GPIO_MODE_OUTPUT_PP,
    .pull = HWD_GPIO_PULL_NONE,
    .speed = HWD_GPIO_SPEED_LOW
};
HWD_GPIO_Init(&gpio_handle, &gpio_config);

// Конфігурація гальм
Brake_Config_t brake_config = {
    .gpio = &gpio_handle,
    .pin = BRAKE_CTRL_Pin,
    .active_high = true,           // HIGH = гальма активні
    .release_delay_ms = 100,       // 100 мс затримка
    .engage_timeout_ms = 3000      // 3 сек таймаут
};

// Ініціалізація
Brake_Init(&brake_driver, &brake_config);
```

#### Brake_Release
Відпускання гальм (дозвіл руху).

```c
Servo_Status_t Brake_Release(Brake_Driver_t* brake);
```

**Приклад:**
```c
Brake_Release(&brake_driver);  // Відпустити гальма
```

#### Brake_Engage
Активація гальм (блокування руху).

```c
Servo_Status_t Brake_Engage(Brake_Driver_t* brake);
```

**Приклад:**
```c
Brake_Engage(&brake_driver);   // Активувати гальма
```

#### Brake_Update
Оновлення стану (викликати періодично).

```c
Servo_Status_t Brake_Update(Brake_Driver_t* brake);
```

**Приклад:**
```c
// У головному циклі
while (1) {
    Brake_Update(&brake_driver);
    HAL_Delay(1);
}
```

#### Brake_NotifyActivity
Повідомлення про активність (скидає таймер).

```c
Servo_Status_t Brake_NotifyActivity(Brake_Driver_t* brake);
```

**Приклад:**
```c
// При команді руху
Servo_SetPosition(&servo, target);
Brake_NotifyActivity(&brake_driver);  // Скинути таймер
```

#### Brake_EmergencyEngage
Аварійна активація гальм.

```c
Servo_Status_t Brake_EmergencyEngage(Brake_Driver_t* brake);
```

**Приклад:**
```c
// При аварійній ситуації
Brake_EmergencyEngage(&brake_driver);  // Миттєво блокувати
```

## Інтеграція з Servo_Controller

Драйвер гальм повністю інтегрований в контролер сервоприводу.

### Ініціалізація

```c
// 1. Створити драйвер гальм
Brake_Driver_t brake_driver;
Brake_Config_t brake_config = {
    .gpio = &gpio_handle,
    .pin = BRAKE_CTRL_Pin,
    .active_high = true,
    .release_delay_ms = 100,
    .engage_timeout_ms = 3000
};
Brake_Init(&brake_driver, &brake_config);

// 2. Конфігурація сервоконтролера
Servo_Config_t servo_config = {
    .axis_config = axis_config,
    .pid_params = pid_params,
    .safety_config = safety_config,
    .traj_params = traj_params,
    .brake_config = brake_config,
    .update_frequency = 1000.0f,
    .enable_brake = true          // Увімкнути гальма
};

// 3. Ініціалізація з гальмами
Servo_InitWithBrake(&servo_controller, &servo_config,
                    &motor_driver.interface, &brake_driver);
```

### Автоматичне керування

Після інтеграції гальма керуються автоматично:

```c
// При команді руху - гальма автоматично відпускаються
Servo_SetPosition(&servo_controller, 90.0f);

// Через 3 сек без руху - гальма автоматично блокують
// (не потрібно робити нічого)

// При аварійній зупинці - гальма миттєво блокують
Servo_EmergencyStop(&servo_controller);
```

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

#### 1. Електромагнітні гальма
```
Control Signal (PA8):
  HIGH → Живлення подано → Гальма ВІДПУЩЕНІ
  LOW  → Живлення відсутнє → Гальма АКТИВНІ (пружина)
```

#### 2. Fail-Safe гальма
```
Control Signal (PA8):
  HIGH → Електромагніт → Гальма АКТИВНІ
  LOW  → Без струму → Гальма ВІДПУЩЕНІ (пружина)

Для цього типу встановіть: active_high = false
```

### GPIO налаштування в CubeMX

**BRAKE_CTRL (PA8):**
- GPIO mode: Output Push Pull
- Pull-up/Pull-down: No pull
- Output speed: Low
- Initial state: High (гальма активні)
- User Label: `BRAKE_CTRL`

## Налаштування параметрів

### Рекомендовані значення

```c
Brake_Config_t brake_config = {
    .gpio = &gpio_handle,
    .pin = BRAKE_CTRL_Pin,
    .active_high = true,

    // Для швидких рухів
    .release_delay_ms = 50,        // Швидке відпускання
    .engage_timeout_ms = 1000,     // Швидке блокування

    // Для плавних рухів (рекомендовано)
    .release_delay_ms = 100,       // Середня швидкість
    .engage_timeout_ms = 3000,     // Середній таймаут

    // Для повільних рухів
    .release_delay_ms = 200,       // Повільне відпускання
    .engage_timeout_ms = 5000      // Довгий таймаут
};
```

### Підбір затримки відпускання

Затримка потрібна для стабілізації:
- **50-100 мс** - стандартні електромагнітні гальма
- **100-200 мс** - великі важкі гальма
- **200-500 мс** - гідравлічні/пневматичні гальма

### Підбір таймауту блокування

Таймаут залежить від режиму роботи:
- **1-2 сек** - швидкі послідовні рухи
- **3-5 сек** - нормальна робота
- **5-10 сек** - рідкісні рухи з тривалими паузами

## Режими роботи

### Автоматичний режим (рекомендовано)

```c
Brake_SetMode(&brake_driver, BRAKE_MODE_AUTO);

// Гальма керуються автоматично:
// - Відпускаються при русі
// - Блокують після таймауту
```

### Ручний режим

```c
Brake_SetMode(&brake_driver, BRAKE_MODE_MANUAL);

// Повне ручне керування:
Brake_Release(&brake_driver);   // Відпустити
// ... виконати рух ...
Brake_Engage(&brake_driver);    // Заблокувати
```

## Діагностика

### Перевірка стану

```c
// Отримати стан гальм
Brake_State_t state = Brake_GetState(&brake_driver);

if (state == BRAKE_STATE_ENGAGED) {
    // Гальма активні
} else {
    // Гальма відпущені
}

// Або скорочений варіант
if (Brake_IsEngaged(&brake_driver)) {
    // Гальма блокують
}
```

### Типові проблеми

| Проблема | Можлива причина | Рішення |
|----------|----------------|---------|
| Гальма не відпускають | Неправильна полярність | Змінити `active_high` |
| Двигун не обертається | Гальма активні | Перевірити стан, викликати `Brake_NotifyActivity()` |
| Блокування надто швидке | Малий таймаут | Збільшити `engage_timeout_ms` |
| Затримка при старті | Велика затримка | Зменшити `release_delay_ms` |

## Приклади використання

### Базове використання

```c
// Ініціалізація
Brake_Driver_t brake;
Brake_Config_t config = {
    .gpio = &gpio_handle,
    .pin = BRAKE_CTRL_Pin,
    .active_high = true,
    .release_delay_ms = 100,
    .engage_timeout_ms = 3000
};
Brake_Init(&brake, &config);

// Головний цикл
while (1) {
    Brake_Update(&brake);

    // Ваш код керування
    if (need_to_move) {
        Brake_NotifyActivity(&brake);
        // Рух двигуна...
    }

    HAL_Delay(1);
}
```

### З Servo_Controller

```c
// Всі гальма керуються автоматично!
Servo_SetPosition(&servo, 90.0f);   // Гальма відпускаються
// ... виконується рух ...
// Через 3 сек без команд - гальма блокують автоматично
```

## Технічні характеристики

- **Час реакції**: < 1 мс (програмне)
- **Роздільна здатність таймерів**: 1 мс
- **Підтримувані напруги**: 3.3V логіка (через драйвер)
- **Режими**: Ручний, Автоматичний
- **Fail-Safe**: Так (гальма активні за замовчуванням)

## Безпека

### Критичні вимоги

✅ **Обов'язково:**
- Гальма ЗАВЖДИ активні при відсутності живлення
- Використовувати fail-safe електромагнітні гальма
- Перевіряти стан перед рухом
- Викликати `Brake_Update()` регулярно

❌ **Заборонено:**
- Блокувати виклики `Brake_Update()`
- Використовувати гальма без fail-safe
- Ігнорувати помилки ініціалізації

## Автор

ServoCore Team - Дипломний проект КПІ, 2025

## Ліцензія

MIT License

---

**Готово до використання!** 🔒

Повернутися до: [README.md](README.md) | [MOTOR_DRIVER_EXAMPLE.md](MOTOR_DRIVER_EXAMPLE.md)
