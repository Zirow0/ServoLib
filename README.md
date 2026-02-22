# ServoLib - Бібліотека керування сервоприводом

## Огляд

**ServoLib** - це модульна, переносима бібліотека для керування DC сервоприводами на платформі STM32F4. Бібліотека побудована на принципах **шарової архітектури** з повною абстракцією від апаратного забезпечення, що забезпечує **портативність**, **тестованість** та **масштабованість**.

### Ключові особливості

- 🏗️ **Шарова архітектура** з Hardware Driver Layer (HWD)
- 🔌 **Повна абстракція** від апаратного забезпечення
- 🎛️ **PID регулятори** з anti-windup
- 📊 **Генератор траєкторій** (лінійні, S-криві)
- 🛡️ **Система безпеки** з захистами та обмеженнями
- 🔒 **Fail-safe електронні гальма**
- 📡 **Підтримка магнітних енкодерів** (AS5600)
- 🧪 **Легке тестування** (HWD можна мокати)
- 🚀 **Готово до використання** в реальних проектах

## Архітектура

ServoLib використовує **5-шарову архітектуру**:

```
┌───────────────────────────────────────┐
│     Application (main.c)              │  ← Ваш код
├───────────────────────────────────────┤
│     Control Layer (ctrl/)             │  ← PID, Safety, Trajectory
├───────────────────────────────────────┤
│     Interface Layer (iface/)          │  ← Motor, Sensor, Brake
├───────────────────────────────────────┤
│     Driver Layer (drv/)               │  ← PWM Motor, AS5600, Brake
├───────────────────────────────────────┤
│     Hardware Driver Layer (hwd/)      │  ← PWM, I2C, GPIO, Timer
├───────────────────────────────────────┤
│     Platform Layer (Board/STM32F411/) │  ← STM32 HAL реалізації
└───────────────────────────────────────┘
```

**Переваги:**
- Логіка (ctrl/, iface/, drv/) **не залежить** від мікроконтролера
- Для портування на іншу платформу — достатньо змінити `Board/`
- Легко тестувати без апаратної частини (mock HWD)

## Реалізовані компоненти

### Core (Ядро)
- ✅ `core.h` / `core.c` - Базові типи, enum, структури
- ✅ `config.h` - Глобальні параметри конфігурації

### Interface Layer (Інтерфейси)
- ✅ `iface/motor.h` / `.c` - Уніфікований інтерфейс двигуна
- ✅ `iface/brake.h` / `.c` - Інтерфейс керування гальмами

### Hardware Driver Layer (HWD)
- ✅ `hwd/hwd.h` - Єдиний entry point
- ✅ `hwd/hwd_pwm.h` - Абстракція PWM
- ✅ `hwd/hwd_i2c.h` - Абстракція I2C
- ✅ `hwd/hwd_gpio.h` - Абстракція GPIO
- ✅ `hwd/hwd_timer.h` - Абстракція таймерів

### Driver Layer (Драйвери)
- ✅ `drv/motor/base.h` / `.c` - Базовий драйвер мотора
- ✅ `drv/motor/pwm.h` / `.c` - PWM драйвер DC мотора
- ✅ `drv/position/position.h` / `.c` - Інтерфейс датчика положення
- ✅ `drv/position/as5600.h` / `.c` - Магнітний енкодер AS5600 (12-bit I2C)
- ✅ `drv/position/aeat9922.h` / `.c` - Магнітний енкодер AEAT-9922 (18-bit SPI)
- ✅ `drv/brake/brake.h` / `.c` - Драйвер електронних гальм

### Control Layer (Керування)
- ✅ `ctrl/servo.h` / `.c` - Головний контролер сервоприводу
- ✅ `ctrl/pid.h` / `.c` - PID регулятор з anti-windup
- ✅ `ctrl/pid_mgr.h` / `.c` - Менеджер множинних PID
- ✅ `ctrl/safety.h` / `.c` - Система безпеки
- ✅ `ctrl/traj.h` / `.c` - Генератор траєкторій
- ✅ `ctrl/err.h` / `.c` - Обробка помилок
- ✅ `ctrl/time.h` / `.c` - Керування таймінгами

### Utilities (Утиліти)
- ✅ `util/math.h` / `.c` - Математичні функції
- ✅ `util/buf.h` / `.c` - Кільцевий буфер

### Platform Layer (STM32F411)
- ✅ `Board/STM32F411/board_config.h` - Конфігурація пінів
- ✅ `Board/STM32F411/board.c` - Ініціалізація плати
- ✅ `Board/STM32F411/hwd_pwm.c` - HWD PWM реалізація
- ✅ `Board/STM32F411/hwd_i2c.c` - HWD I2C реалізація
- ✅ `Board/STM32F411/hwd_gpio.c` - HWD GPIO реалізація
- ✅ `Board/STM32F411/hwd_timer.c` - HWD Timer реалізація

### Документація
- ✅ `README.md` - Цей файл (швидкий старт)
- ✅ `structure.md` - Детальна структура проекту
- ✅ `technical_specifications.md` - Технічна специфікація
- ✅ `MOTOR_DRIVER_EXAMPLE.md` - Приклади драйвера мотора
- ✅ `BRAKE_DRIVER.md` - Документація драйвера гальм

## Структура каталогів (спрощено)

```
ServoLib/
├── Inc/                    # Заголовочні файли
│   ├── core.h              # Основні типи
│   ├── config.h            # Конфігурація
│   ├── iface/              # Інтерфейси (motor, sensor, brake)
│   ├── hwd/                # HWD абстракції (pwm, i2c, gpio, timer)
│   ├── drv/                # Драйвери (motor, sensor, brake)
│   ├── ctrl/               # Керування (servo, pid, safety, traj)
│   └── util/               # Утиліти (math, buf)
│
├── Src/                    # Реалізації (структура як Inc/)
│
├── Board/                  # Платформо-специфічний код
│   └── STM32F411/          # Реалізація для STM32F411
│       ├── board_config.h
│       ├── hwd_*.c         # HWD реалізації через STM32 HAL
│       └── board.c
│
└── *.md                    # Документація
```

**Детальна структура:** [structure.md](structure.md)

## Основні можливості

### 🎛️ Керування двигуном
- **PWM керування** (1-100 kHz, типово 1 kHz)
- **Двоканальний режим** для H-bridge (L298N, TB6612)
- **Статистика роботи** (час, кількість запусків, помилки)
- **Інверсія напрямку** (програмна)

### 📡 Датчики положення
- **AS5600** магнітний енкодер (12-біт, 4096 позицій)
- **I2C зв'язок** з таймаутами та повторами
- **Обчислення швидкості** за різницею позицій
- **Фільтрація аномальних значень**

### 🎯 PID регулювання
- **Класичний PID** з коефіцієнтами Kp, Ki, Kd
- **Anti-windup** для інтегральної складової
- **Обмеження виходу** (min/max)
- **Режими:** позиція, швидкість, момент (опціонально)

### 📊 Генератор траєкторій
- **Лінійні траєкторії**
- **S-криві** (трапецеїдальний профіль)
- **Плавні переходи** без стрибків
- **Налаштовувані параметри** (швидкість, прискорення, ривок)

### 🛡️ Система безпеки
- **Обмеження положення** (min/max градуси)
- **Обмеження швидкості** (град/с)
- **Захист від перевантаження** по струму
- **Watchdog** для детекції зависань
- **Аварійна зупинка** (< 10 мс)

### 🔒 Fail-safe гальма
- **Активні за замовчуванням** (без живлення)
- **Автоматичне відпускання** перед рухом
- **Таймаут бездіяльності** (автоматична активація)
- **Аварійна активація** при критичних ситуаціях
- **Налаштовувані затримки** (50-500 мс)

## Швидкий старт

### 1. Налаштування CubeMX

#### TIM3 - PWM для мотора
- Channel 1 (PA6): PWM Generation CH1
- Channel 2 (PA7): PWM Generation CH2
- Prescaler: 99, Period: 999 (1 kHz)

#### I2C1 - Датчик AS5600
- Mode: I2C, Speed: 400 kHz (Fast Mode)
- SCL (PB6), SDA (PB7)

#### TIM5 - Таймер для мікросекунд
- Prescaler: 99, Period: 0xFFFFFFFF (32-біт)

#### GPIO - Керування гальмами
- PA8: GPIO_Output (BRAKE_CTRL)

### 2. Додавання до проекту STM32CubeIDE

**Include paths** (Project Properties → C/C++ Build → Settings → Include paths):
```
../ServoLib/Inc
../ServoLib/Board/STM32F411
```

**Source files** (додати до проекту):
```
ServoLib/Src/**/*.c               # Усі .c файли з Src/
ServoLib/Board/STM32F411/*.c      # Усі .c файли з Board/
```

### 3. Базовий приклад використання

#### Простий драйвер мотора

```c
#include "drv/motor/pwm.h"
#include "Board/STM32F411/board_config.h"

// Глобальні змінні
PWM_Motor_Driver_t motor_driver;

void Motor_Init_Example(void)
{
    // 1. Конфігурація PWM
    PWM_Motor_Config_t motor_config = {
        .type = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd_timer = &htim3,
        .pwm_fwd_channel = TIM_CHANNEL_1,
        .pwm_bwd_timer = &htim3,
        .pwm_bwd_channel = TIM_CHANNEL_2
    };

    // 2. Створення та ініціалізація драйвера
    PWM_Motor_Create(&motor_driver, &motor_config);

    // 3. Параметри двигуна
    Motor_Params_t params = {
        .max_power = 100.0f,
        .min_power = 5.0f,
        .max_current = 2000,
        .invert_direction = false
    };

    PWM_Motor_Init(&motor_driver, &params);
}

void Motor_Control_Example(void)
{
    // Встановити 50% потужності вперед
    PWM_Motor_SetPower(&motor_driver, 50.0f);
    HAL_Delay(2000);

    // Зупинити
    PWM_Motor_Stop(&motor_driver);
    HAL_Delay(500);

    // Встановити 30% потужності назад
    PWM_Motor_SetPower(&motor_driver, -30.0f);
    HAL_Delay(2000);

    // Зупинити
    PWM_Motor_Stop(&motor_driver);
}
```

#### Повний сервоконтролер з гальмами

```c
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/brake/brake.h"

// Глобальні змінні
Servo_Controller_t servo;
PWM_Motor_Driver_t motor;
Brake_Driver_t brake;

void Servo_Init_Example(void)
{
    // 1. Ініціалізація мотора (див. вище)
    Motor_Init_Example();

    // 2. Ініціалізація гальм
    Brake_Config_t brake_config = {
        .gpio_port = BRAKE_CTRL_GPIO_Port,
        .gpio_pin = BRAKE_CTRL_Pin,
        .active_high = true,
        .release_delay_ms = 100,
        .engage_timeout_ms = 3000
    };
    Brake_Init(&brake, &brake_config);

    // 3. Конфігурація сервоконтролера
    Servo_Config_t servo_config = {
        .update_frequency = 1000.0f,  // 1 kHz
        .enable_brake = true,
        // ... інші параметри (PID, safety, trajectory)
    };

    // 4. Ініціалізація сервоконтролера
    Servo_InitWithBrake(&servo, &servo_config,
                        &motor.interface, &brake);
}

void Servo_Control_Example(void)
{
    // Встановити цільове положення 90°
    Servo_SetPosition(&servo, 90.0f);

    // Головний цикл
    while (1) {
        Servo_Update(&servo);  // Оновлення контролера

        // Перевірка досягнення цілі
        if (Servo_IsAtTarget(&servo)) {
            // Ціль досягнуто!
        }

        HAL_Delay(1);  // 1 kHz
    }
}
```

## API

### Основні функції керування

#### Servo Controller

```c
// Ініціалізація
Servo_Status_t Servo_Init(Servo_Controller_t* servo,
                          const Servo_Config_t* config,
                          Motor_Interface_t* motor);

Servo_Status_t Servo_InitWithBrake(Servo_Controller_t* servo,
                                    const Servo_Config_t* config,
                                    Motor_Interface_t* motor,
                                    Brake_Driver_t* brake);

// Основний цикл (викликати періодично)
Servo_Status_t Servo_Update(Servo_Controller_t* servo);

// Керування
Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position);
Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity);
Servo_Status_t Servo_Stop(Servo_Controller_t* servo);
Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo);

// Читання стану
float Servo_GetPosition(const Servo_Controller_t* servo);
float Servo_GetVelocity(const Servo_Controller_t* servo);
Servo_State_t Servo_GetState(const Servo_Controller_t* servo);
bool Servo_IsAtTarget(const Servo_Controller_t* servo);
```

#### PWM Motor Driver

```c
// Створення та ініціалізація
Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                 const PWM_Motor_Config_t* config);

Servo_Status_t PWM_Motor_Init(PWM_Motor_Driver_t* driver,
                               const Motor_Params_t* params);

// Керування потужністю (-100.0 до +100.0)
Servo_Status_t PWM_Motor_SetPower(PWM_Motor_Driver_t* driver, float power);

// Зупинка
Servo_Status_t PWM_Motor_Stop(PWM_Motor_Driver_t* driver);
Servo_Status_t PWM_Motor_EmergencyStop(PWM_Motor_Driver_t* driver);

// Статистика
Servo_Status_t PWM_Motor_GetStats(PWM_Motor_Driver_t* driver,
                                   Motor_Stats_t* stats);
```

#### Brake Driver

```c
// Ініціалізація
Servo_Status_t Brake_Init(Brake_Driver_t* brake,
                          const Brake_Config_t* config);

// Керування
Servo_Status_t Brake_Release(Brake_Driver_t* brake);
Servo_Status_t Brake_Engage(Brake_Driver_t* brake);
Servo_Status_t Brake_EmergencyEngage(Brake_Driver_t* brake);

// Оновлення (викликати періодично)
Servo_Status_t Brake_Update(Brake_Driver_t* brake);
Servo_Status_t Brake_NotifyActivity(Brake_Driver_t* brake);

// Читання стану
Brake_State_t Brake_GetState(const Brake_Driver_t* brake);
bool Brake_IsEngaged(const Brake_Driver_t* brake);
```

## Детальна документація

| Документ | Опис |
|----------|------|
| **[technical_specifications.md](technical_specifications.md)** | Повна технічна специфікація (вимоги, архітектура, параметри) |
| **[structure.md](structure.md)** | Детальна структура проекту (каталоги, файли, залежності) |
| **[MOTOR_DRIVER_EXAMPLE.md](MOTOR_DRIVER_EXAMPLE.md)** | Приклади використання драйвера мотора |
| **[BRAKE_DRIVER.md](BRAKE_DRIVER.md)** | Документація та приклади драйвера гальм |
| **[Examples/MOTOR_PWM_DIR_GUIDE.md](Examples/MOTOR_PWM_DIR_GUIDE.md)** | Керування двигуном через PWM + DIR драйвер |

## Портування на інші STM32

**Крок 1:** Створіть новий каталог для вашої платформи
```bash
mkdir -p ServoLib/Board/STM32F407  # Приклад для F407
```

**Крок 2:** Скопіюйте файли з існуючої платформи
```bash
cp -r ServoLib/Board/STM32F411/* ServoLib/Board/STM32F407/
```

**Крок 3:** Адаптуйте `board_config.h` (піни, таймери)
```c
// board_config.h для STM32F407
#define MOTOR_PWM_TIMER      htim4  // Змінено з TIM3 на TIM4
#define MOTOR_PWM_CHANNEL_FWD TIM_CHANNEL_3
#define MOTOR_PWM_CHANNEL_BWD TIM_CHANNEL_4
// ... інші зміни
```

**Крок 4:** Перекомпілюйте проект

**Вся логіка (ctrl/, iface/, drv/) залишається без змін!**

## Стан розробки

### Реалізовано ✅
- [x] Шарова архітектура з HWD
- [x] Драйвер PWM мотора
- [x] Драйвер магнітного енкодера AS5600
- [x] Драйвер електронних гальм з fail-safe
- [x] PID регулятор з anti-windup
- [x] Генератор траєкторій (лінійні, S-криві)
- [x] Система безпеки та захистів
- [x] Калібрування положення
- [x] Повна документація

### В процесі 🚧
- [ ] Тестування на реальному обладнанні
- [ ] Налаштування PID параметрів
- [ ] Оптимізація продуктивності

### Заплановано 📋
- [ ] Підтримка степпер-моторів
- [ ] BLDC драйвер
- [ ] Підтримка RTOS (FreeRTOS)
- [ ] CAN інтерфейс

## Технічні характеристики

| Параметр | Значення |
|----------|----------|
| **MCU** | STM32F411CEU6 (BlackPill) |
| **Тактова частота** | 100 MHz |
| **Flash** | 512 KB |
| **RAM** | 128 KB |
| **Частота PWM** | 1 kHz (налаштовується до 100 kHz) |
| **Роздільна здатність PWM** | 1000 кроків (0.1%) |
| **Частота контуру керування** | 1 kHz (1 мс період) |
| **Підтримувані H-bridge** | L298N, TB6612, DRV8833, тощо |
| **Датчик положення** | AS5600 (12-біт, I2C) |
| **Стандарт коду** | C99 |
| **Використання пам'яті** | Статична (без malloc) |

## Вимоги

### Апаратні
- **MCU:** STM32F411CEU6 (BlackPill) або аналогічна
- **Мотор:** DC мотор з редуктором, до 2A
- **H-bridge:** L298N, TB6612 або аналогічний
- **Енкодер:** AS5600 магнітний енкодер (опціонально)
- **Гальма:** Електромагнітні fail-safe гальма (опціонально)
- **Живлення:** Окреме джерело для мотора (6-24V)

### Програмні
- **IDE:** STM32CubeIDE 1.10 або новіший
- **Toolchain:** ARM GCC
- **HAL:** STM32F4 HAL Library
- **OS:** Bare-metal (без RTOS)

## Контрибуція

Проект знаходиться в активній розробці. Вітаються:
- Звіти про помилки
- Пропозиції покращень
- Pull requests з новими функціями
- Документація та приклади

## Автори та ліцензія

**Розробники:** ServoCore Team
**Організація:** КПІ ім. Ігоря Сікорського
**Рік:** 2025
**Ліцензія:** MIT License

---

## Корисні посилання

- 📖 [Технічна специфікація](technical_specifications.md)
- 🏗️ [Структура проекту](structure.md)
- 🚗 [Приклади драйвера мотора](MOTOR_DRIVER_EXAMPLE.md)
- 🔒 [Документація гальм](BRAKE_DRIVER.md)
- 📊 STM32F411 Reference Manual
- 📡 AS5600 Datasheet

---

**Готово до використання в реальних проектах!** 🚀
