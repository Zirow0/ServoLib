# Структура бібліотеки сервоприводу

## Дерево каталогів

```
ServoLib/
├── Inc/                        # Заголовочні файли
│   ├── core.h                  # Основні типи, enum, структури
│   ├── config.h                # Включення системи конфігурації
│   ├── config_lib.h            # Константи бібліотеки (не змінюються)
│   ├── config_defaults.h       # Значення за замовчуванням
│   ├── config_user.h           # Користувацькі налаштування (проект-специфічні)
│   │
│   ├── hwd/                    # Шар драйверів апаратного забезпечення (HWD)
│   │   ├── hwd.h               # Єдиний фасад для всіх HWD-функцій
│   │   ├── hwd_pwm.h           # Абстракція PWM-сигналів
│   │   ├── hwd_i2c.h           # Абстракція I2C-зв'язку
│   │   ├── hwd_spi.h           # Абстракція SPI-зв'язку
│   │   ├── hwd_timer.h         # Абстракція таймерів та затримок
│   │   └── hwd_gpio.h          # Абстракція GPIO
│   │
│   ├── drv/                    # Апаратні драйвери (використовують HWD)
│   │   ├── motor/
│   │   │   ├── motor.h         # Універсальний інтерфейс з hardware callbacks
│   │   │   └── pwm.h           # PWM драйвер (DC motors)
│   │   │
│   │   ├── position/
│   │   │   ├── position.h      # Універсальний інтерфейс position sensor
│   │   │   ├── aeat9922.h      # AEAT-9922 18-bit SPI encoder
│   │   │   └── as5600.h        # AS5600 12-bit I2C encoder
│   │   │
│   │   └── brake/
│   │       ├── brake.h         # Універсальний інтерфейс brake з callbacks
│   │       └── gpio_brake.h    # GPIO драйвер електромагнітних гальм
│   │
│   ├── ctrl/                   # Високорівнева логіка керування
│   │   ├── servo.h             # Головний контролер сервоприводу
│   │   ├── pid.h               # PID-регулятор
│   │   ├── pid_mgr.h           # Менеджер кількох PID
│   │   ├── safety.h            # Система безпеки (межі, струм, watchdog)
│   │   ├── traj.h              # Генератор траєкторій
│   │   ├── err.h               # Обробка помилок та кодів
│   │   ├── time.h              # Керування таймінгами та частотою
│   │   └── pid_mgr.h           # Менеджер PID регуляторів (каскадне керування)
│   │
│   └── util/                   # Універсальні допоміжні утиліти
│       ├── math.h              # Математичні функції
│       ├── buf.h               # Кільцевий буфер
│       ├── checksum.h          # CRC-8 для SPI протоколів
│       ├── derivative.h        # Обчислення швидкості (похідна позиції)
│       └── prediction.h        # Екстраполяція позиції між оновленнями
│
├── Src/                        # Реалізації
│   ├── core.c
│   ├── hwd/
│   │   └── hwd.c               # Загальні HWD функції
│   ├── drv/
│   │   ├── motor/
│   │   │   ├── motor.c         # Базова логіка motor interface
│   │   │   └── pwm.c           # PWM hardware callbacks
│   │   ├── position/
│   │   │   ├── position.c      # Базова логіка position sensor
│   │   │   ├── aeat9922.c      # AEAT-9922 hardware callbacks
│   │   │   └── as5600.c        # AS5600 hardware callbacks
│   │   └── brake/
│   │       ├── brake.c         # Базова логіка brake (state machine)
│   │       └── gpio_brake.c    # GPIO hardware callbacks
│   ├── ctrl/
│   │   ├── servo.c
│   │   ├── pid.c
│   │   ├── pid_mgr.c
│   │   ├── safety.c
│   │   ├── traj.c
│   │   ├── err.c
│   │   ├── time.c
│   │   └── pid_mgr.c
│   └── util/
│       ├── math.c
│       ├── buf.c
│       ├── checksum.c          # CRC-8 реалізація
│       ├── derivative.c        # Velocity calculation
│       └── prediction.c        # Position extrapolation
│
├── Board/                      # Платформо-залежні реалізації HWD
│   └── STM32F411/              # Реалізація для STM32F411
│       ├── board_config.h      # Визначення пінів, таймерів, периферії
│       ├── board.c             # Ініціалізація плати
│       ├── hwd_pwm.c           # Реалізація PWM через STM32 HAL
│       ├── hwd_i2c.c           # Реалізація I2C через STM32 HAL
│       ├── hwd_spi.c           # Реалізація SPI через STM32 HAL
│       ├── hwd_timer.c         # Реалізація таймерів (GetTick, Delay)
│       └── hwd_gpio.c          # Реалізація GPIO операцій
│
├── Doc/                        # Документація
│   ├── structure.md            # Цей файл - Структура проекту
│   ├── technical_specifications.md # Технічна специфікація
│   ├── BRAKE_DRIVER.md         # Документація драйвера гальм
│   └── AEAT-9922/
│       └── CONFIGURATION_GUIDE.md # Конфігурація AEAT-9922
│
├── Examples/                   # Приклади використання
│   ├── aeat9922_quick_start.c
│   └── aeat9922_spi4_abi_example.c
│
├── Templates/                  # Шаблони конфігурації
│   └── config_user_template.h  # Шаблон користувацької конфігурації
│
├── README.md                   # Документація - Швидкий старт
└── CLAUDE.md                   # Інструкції для Claude Code
```

---

## Опис каталогів

| Каталог | Призначення |
|---------|-------------|
| `Inc/` | Заголовочні файли — інтерфейси, декларації, структури даних |
| `Src/` | Реалізації `.c` файлів (відповідність 1:1 з `.h`) |
| `hwd/` | **Hardware Driver Layer** — єдиний шар абстракції апаратної частини. Не конфліктує з STM32 HAL |
| `drv/` | Драйвери периферії — універсальні інтерфейси з **hardware callbacks**, використовують **HWD** |
| `ctrl/` | Високорівнева логіка керування — **не знає про MCU**, працює через `drv/` та `hwd/` |
| `util/` | Універсальні утиліти — математика, буфери, допоміжні функції |
| `Board/` | **Платформо-специфічний код** — єдина частина проекту, що знає про конкретний мікроконтролер |
| `Doc/` | Документація проекту |
| `Examples/` | Приклади використання |
| `Templates/` | Шаблони конфігураційних файлів |

---

## Опис файлів

### Core (Ядро системи)

| Файл | Опис |
|------|------|
| `core.h` / `core.c` | Базові типи даних (`Servo_Status_t`, `Motor_Type_t`, `Axis_Config_t`), enum, глобальні структури |
| `config.h` | Включення системи конфігурації (config_lib.h → config_user.h → config_defaults.h) |
| `config_lib.h` | Константи бібліотеки (PI, конверсії одиниць) - не змінюються |
| `config_defaults.h` | Значення за замовчуванням (можуть бути перевизначені в config_user.h) |
| `config_user.h` | Користувацькі налаштування проекту (створюється з Templates/config_user_template.h) |

### hwd/ (Hardware Driver Layer)

| Файл | Опис | Ключові функції |
|------|------|-----------------|
| `hwd.h` / `hwd.c` | Єдиний entry point для всіх HWD функцій | Включає всі hwd_*.h |
| `hwd_pwm.h` | Абстракція PWM-сигналів | `HWD_PWM_Init()`, `HWD_PWM_SetDuty()`, `HWD_PWM_Start()` |
| `hwd_i2c.h` | Абстракція I2C-зв'язку | `HWD_I2C_Init()`, `HWD_I2C_ReadReg()`, `HWD_I2C_WriteReg()` |
| `hwd_spi.h` | Абстракція SPI-зв'язку | `HWD_SPI_Init()`, `HWD_SPI_TransmitReceive()` |
| `hwd_timer.h` | Абстракція таймерів | `HWD_Timer_GetMillis()`, `HWD_Timer_GetMicros()`, `HWD_Timer_DelayMs()` |
| `hwd_gpio.h` | Абстракція GPIO | `HWD_GPIO_Init()`, `HWD_GPIO_WritePin()`, `HWD_GPIO_ReadPin()` |

### drv/motor/ (Драйвери двигуна)

| Файл | Опис |
|------|------|
| `motor.h` / `motor.c` | Універсальний інтерфейс з hardware callbacks, базова логіка (state, power, stats) |
| `pwm.h` / `pwm.c` | PWM драйвер DC двигуна (hardware callbacks для motor interface) |

**Архітектура:** Hardware Callbacks Pattern - базова логіка в `motor.c`, апаратні особливості в callbacks

### drv/position/ (Драйвери сенсорів позиції)

| Файл | Опис |
|------|------|
| `position.h` / `position.c` | Універсальний інтерфейс з hardware callbacks (velocity, multi-turn, prediction) |
| `aeat9922.h` / `aeat9922.c` | AEAT-9922 18-bit SPI encoder (hardware callbacks: read_raw, calibrate) |
| `as5600.h` / `as5600.c` | AS5600 12-bit I2C encoder (hardware callbacks: read_raw, calibrate) |

**Архітектура:** Hardware Callbacks Pattern - обробка даних в `position.c`, читання апаратури в callbacks

### drv/brake/ (Драйвер гальм)

| Файл | Опис |
|------|------|
| `brake.h` / `brake.c` | Універсальний інтерфейс з hardware callbacks, state machine (ENGAGED/RELEASED/ENGAGING/RELEASING) |
| `gpio_brake.h` / `gpio_brake.c` | GPIO драйвер електромагнітних гальм (hardware callbacks для brake interface) |

**Архітектура:** Hardware Callbacks Pattern - state machine в `brake.c`, GPIO операції в callbacks

### ctrl/ (Система керування)

| Файл | Опис | Ключові функції |
|------|------|-----------------|
| `servo.h` / `servo.c` | Головний контролер сервоприводу | `Servo_Init()`, `Servo_Update()`, `Servo_SetPosition()` |
| `pid.h` / `pid.c` | PID регулятор з anti-windup | `PID_Init()`, `PID_Compute()`, `PID_Reset()` |
| `pid_mgr.h` / `pid_mgr.c` | Менеджер для координації кількох PID регуляторів | `PID_Manager_Init()`, `PID_Manager_Update()` |
| `safety.h` / `safety.c` | Система безпеки (обмеження, захисти) | `Safety_CheckLimits()`, `Safety_HandleError()` |
| `traj.h` / `traj.c` | Генератор траєкторій руху | `Trajectory_Generate()`, `Trajectory_Update()` |
| `err.h` / `err.c` | Обробка помилок та логування | `Error_Log()`, `Error_GetLast()`, `Error_Clear()` |
| `time.h` / `time.c` | Керування таймінгами та частотою оновлення | `Timer_Init()`, `Timer_Elapsed()` |
| `pid_mgr.h` / `pid_mgr.c` | Менеджер PID регуляторів (каскадне керування) | `PID_Manager_Add()`, `PID_Manager_UpdateAll()` |

### util/ (Утиліти)

| Файл | Опис |
|------|------|
| `math.h` / `math.c` | Математичні функції (інтерполяція, нормалізація, фільтри) |
| `buf.h` / `buf.c` | Кільцевий буфер для потоків даних |
| `checksum.h` / `checksum.c` | CRC-8 для перевірки даних SPI протоколів (AEAT-9922) |
| `derivative.h` / `derivative.c` | Обчислення швидкості як похідної позиції (wraparound-safe) |
| `prediction.h` / `prediction.c` | Екстраполяція позиції між оновленнями сенсора |

### Board/STM32F411/ (Платформо-специфічний код)

| Файл | Опис |
|------|------|
| `board_config.h` | Визначення пінів, таймерів, I2C, SPI, периферії для конкретної плати |
| `board.c` | Ініціалізація плати та периферії |
| `hwd_pwm.c` | Реалізація HWD PWM через STM32 HAL (`stm32f4xx_hal_tim.h`) |
| `hwd_i2c.c` | Реалізація HWD I2C через STM32 HAL (`stm32f4xx_hal_i2c.h`) |
| `hwd_spi.c` | Реалізація HWD SPI через STM32 HAL (`stm32f4xx_hal_spi.h`) |
| `hwd_timer.c` | Реалізація HWD Timer через STM32 HAL (SysTick або TIM) |
| `hwd_gpio.c` | Реалізація HWD GPIO через STM32 HAL (`stm32f4xx_hal_gpio.h`) |

---

## Ключові переваги архітектури

### 1. Hardware Callbacks Pattern
Нова архітектура драйверів з розділенням базової логіки та апаратних операцій:
- **Базова логіка** в `motor.c`, `position.c`, `brake.c` - обробка даних, state machine, обчислення
- **Апаратні callbacks** в `pwm.c`, `aeat9922.c`, `gpio_brake.c` - читання/запис апаратури
- **Легко додавати нові типи** - просто реалізувати callbacks для нової апаратури

### 2. Відсутність конфліктів
HWD шар не конфліктує з STM32 HAL бібліотекою. HAL використовується лише в `Board/`, а вся логіка працює через HWD абстракції.

### 3. Портативність
Вся логіка (ctrl/, drv/) повністю ізольована від апаратного забезпечення. Для портування на іншу платформу STM32:
- Створити новий каталог `Board/STM32Fxxx/`
- Реалізувати HWD функції для нової платформи
- Готово! Жоден рядок логіки не змінюється

### 4. Масштабованість
- Легко додати нові драйвери двигунів (stepper, BLDC) - реалізувати hardware callbacks
- Легко додати нові датчики (оптичні енкодери, резольвери, SSI) - реалізувати read_raw callback
- Легко розширити систему до багатоосьового контролера

### 5. Тестованість
HWD можна легко замокати для unit-тестів:
```c
// Mock HWD для тестування без апаратної частини
HWD_PWM_SetDuty_mock(...);
HWD_SPI_TransmitReceive_mock(...);
```

### 6. Читабельність
Чітке розділення відповідальності між шарами:
- **Application** → викликає ctrl/
- **ctrl/** → використовує drv/
- **drv/** → використовує hwd/ (через hardware callbacks)
- **hwd/** → реалізується в Board/
- **Board/** → використовує STM32 HAL

## Потік виконання

### Приклад 1: Встановлення положення сервоприводу

```
User Application (main.c)
    ↓
Servo_SetPosition() [ctrl/servo.c]
    ↓
PID_Compute() [ctrl/pid.c]
    ↓
Motor_SetPower() [drv/motor/motor.c] - базова логіка
    ↓
motor->hw.set_power() [drv/motor/pwm.c] - hardware callback
    ↓
HWD_PWM_SetDuty() [hwd/hwd_pwm.h]
    ↓
HWD_PWM_SetDuty() [Board/STM32F411/hwd_pwm.c]
    ↓
STM32 HAL (stm32f4xx_hal_tim.c)
```

### Приклад 2: Зчитування положення з енкодера

```
User Application (main.c)
    ↓
Servo_Update() [ctrl/servo.c]
    ↓
Position_Sensor_Update() [drv/position/position.c] - базова логіка
    ↓
sensor->hw.read_raw() [drv/position/aeat9922.c] - hardware callback
    ↓
HWD_SPI_TransmitReceive() [hwd/hwd_spi.h]
    ↓
HWD_SPI_TransmitReceive() [Board/STM32F411/hwd_spi.c]
    ↓
STM32 HAL (stm32f4xx_hal_spi.c)
    ↓
[Повернення до position.c]
    ↓
RawToAngleDegrees() - конвертація raw → degrees
    ↓
Derivative_CalculateVelocity() - обчислення швидкості
    ↓
Prediction_GetCurrentPosition() - екстраполяція
```

## Залежності між модулями

```
┌───────────────────────────────────────────────────┐
│         Application Layer (main.c)                │
└───────────────────┬───────────────────────────────┘
                    │ залежить від
                    ↓
┌───────────────────────────────────────────────────┐
│         ctrl/ (Control Layer)                     │
│  - Не знає про апаратне забезпечення              │
│  - Працює з universal interfaces (drv/)           │
└───────────────────┬───────────────────────────────┘
                    │ використовує
                    ↓
┌───────────────────────────────────────────────────┐
│      drv/ (Driver Layer + Hardware Callbacks)     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Базова логіка (motor.c, position.c,        │  │
│  │ brake.c) - state, обробка даних             │  │
│  └───────────┬─────────────────────────────────┘  │
│              │ викликає callbacks                  │
│              ↓                                     │
│  ┌─────────────────────────────────────────────┐  │
│  │ Hardware callbacks (pwm.c, aeat9922.c,      │  │
│  │ gpio_brake.c) - операції з апаратурою       │  │
│  └───────────┬─────────────────────────────────┘  │
└──────────────┼───────────────────────────────────┘
               │ використовує
               ↓
┌───────────────────────────────────────────────────┐
│         hwd/ (Hardware Driver Layer)              │
│  - Абстрактні декларації функцій                  │
└───────────────────┬───────────────────────────────┘
                    │ реалізується в
                    ↓
┌───────────────────────────────────────────────────┐
│      Board/STM32F411/ (Platform)                  │
│  - Єдине місце із залежністю від MCU              │
│  - Реалізація HWD через STM32 HAL                 │
└───────────────────────────────────────────────────┘
```

---

## Додаткова інформація

**Детальна документація:**
- [../README.md](../README.md) - Швидкий старт та основи використання
- [technical_specifications.md](technical_specifications.md) - Повна технічна специфікація
- [BRAKE_DRIVER.md](BRAKE_DRIVER.md) - Документація системи гальм
- [AEAT-9922/CONFIGURATION_GUIDE.md](AEAT-9922/CONFIGURATION_GUIDE.md) - Конфігурація AEAT-9922
- [../CLAUDE.md](../CLAUDE.md) - Інструкції для Claude Code

**Готово до:**
- ✅ Портування на інші STM32
- ✅ Unit та інтеграційного тестування
- ✅ Масштабування (додавання нових компонентів)
- ✅ Використання в реальних проектах

---

**Версія:** 1.0
**Дата оновлення:** 2025
**Автори:** ServoCore Team, Дипломний проект КПІ
**Ліцензія:** MIT License
