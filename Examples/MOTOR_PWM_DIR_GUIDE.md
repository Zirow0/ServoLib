# Керування двигуном через PWM + DIR драйвер

## Огляд

Цей посібник показує як використовувати драйвер двигуна ServoLib для контролерів, які використовують:
- **Один PWM канал** - для керування швидкістю обертання
- **Один GPIO канал** - для вибору напрямку обертання (DIR)

Такий тип керування підтримують драйвери: **TB6612FNG**, **DRV8833** (в режимі PWM/DIR), **L293D**, **L9110S**.

## Переваги режиму PWM + DIR

| Переваги | Опис |
|----------|------|
| Економія пінів | Потрібно лише 2 піни замість 2-3 для інших режимів |
| Простота підключення | Легко з'єднати з контролером |
| Ефективність | Менше навантаження на MCU (один PWM таймер) |
| Швидке перемикання | Напрямок змінюється GPIO без затримок PWM |

## Схема підключення

### Підключення до STM32F411 (BlackPill)

```
STM32F411CEU6                      TB6612FNG / DRV8833
┌──────────────┐                  ┌─────────────────┐
│              │                  │                 │
│ PA6 (TIM3CH1)├─────────────────►│ PWM/PWMA       │
│              │    PWM сигнал    │                 │
│              │                  │                 │
│ PA0 (GPIO)   ├─────────────────►│ DIR/AIN1       │
│              │   Напрямок       │                 │
│              │                  │                 │
│         GND  ├─────────────────►│ GND            │
│              │                  │                 │
└──────────────┘                  │                 │
                                  │ OUT1 ├─────┐    │
                                  │ OUT2 ├──┐  │    │
                                  └──────┴──┼──┼────┘
                                            │  │
                                        ┌───▼──▼────┐
                                        │  DC Motor  │
                                        └────────────┘
```

### Таблиця з'єднань

| STM32 Pin | Функція | Драйвер Pin | Опис |
|-----------|---------|-------------|------|
| PA6 | TIM3 CH1 | PWM/PWMA | PWM сигнал для швидкості (0-100%) |
| PA0 | GPIO Output | DIR/AIN1 | HIGH = вперед, LOW = назад |
| GND | Ground | GND | Спільна земля |
| +5V | Power (optional) | VCC | Живлення логіки драйвера |

### Підключення живлення мотора

```
┌──────────────┐
│ Зовнішнє     │
│ джерело      │
│ 6-12V DC     │
│              │
│ +    -       │
└──┬────┬──────┘
   │    │
   │    └─────────► GND драйвера (та STM32 GND)
   │
   └──────────────► VM/VIN драйвера
```

**ВАЖЛИВО**: Завжди з'єднуйте GND STM32 та GND джерела живлення мотора!

## Налаштування CubeMX

### 1. Налаштування PWM (TIM3)

**TIM3 Configuration:**
- Clock Source: Internal Clock
- Channel 1: PWM Generation CH1
- Mode: PWM mode 1

**Parameter Settings:**
```
Prescaler: 99
Counter Mode: Up
Counter Period (ARR): 999
auto-reload preload: Enable
```

**Розрахунок частоти:**
```
Timer Clock = 100 MHz (STM32F411)
PWM Frequency = Timer Clock / ((Prescaler + 1) × (Period + 1))
PWM Frequency = 100,000,000 / ((99 + 1) × (999 + 1)) = 1 kHz
```

**PWM Output pins:**
- CH1: PA6

### 2. Налаштування GPIO для DIR

**GPIO Configuration:**
- Pin: PA0
- Mode: GPIO_Output
- GPIO output level: Low
- Mode: Output Push Pull
- Pull-up / Pull-down: No pull-up and no pull-down
- Maximum output speed: Low
- User Label: MOTOR_DIR

### 3. GPIO Code Generation

В Project Manager → Code Generator:
- ✅ Generate peripheral initialization as a pair of '.c/.h' files per peripheral
- ✅ Keep User Code when re-generating

## Структура проекту

```
ServoCore/
├── Core/
│   ├── Src/
│   │   └── main.c              # Ваш головний файл
│   └── Inc/
│       └── main.h
│
├── ServoLib/
│   ├── Inc/
│   │   ├── drv/motor/
│   │   │   └── pwm.h           # Драйвер PWM мотора
│   │   ├── hwd/
│   │   │   └── hwd_pwm.h       # HWD абстракція PWM
│   │   └── core.h
│   │
│   ├── Src/
│   │   └── drv/motor/
│   │       └── pwm.c
│   │
│   ├── Board/STM32F411/
│   │   ├── board_config.h      # Конфігурація пінів
│   │   └── hwd_pwm.c           # Реалізація HWD PWM
│   │
│   └── Examples/
│       ├── motor_pwm_dir_example.c    # Приклад коду
│       └── MOTOR_PWM_DIR_GUIDE.md     # Цей документ
```

## Приклад використання

### Базовий приклад (main.c)

```c
/* Includes */
#include "main.h"
#include "drv/motor/pwm.h"
#include "Board/STM32F411/board_config.h"

/* Private variables */
HWD_PWM_Handle_t pwm_handle;     // PWM канал
PWM_Motor_Driver_t motor;        // Драйвер мотора

/* Private functions */
void Motor_PWM_DIR_Init(void);
void Motor_Simple_Test(void);

int main(void)
{
    /* MCU Initialization */
    HAL_Init();
    SystemClock_Config();

    /* Peripheral Initialization (згенеровано CubeMX) */
    MX_GPIO_Init();
    MX_TIM3_Init();

    /* Motor Driver Initialization */
    Motor_PWM_DIR_Init();

    /* Infinite loop */
    while (1)
    {
        Motor_Simple_Test();
        HAL_Delay(5000);  // 5 секунд пауза між циклами
    }
}

/**
 * @brief Ініціалізація драйвера мотора
 */
void Motor_PWM_DIR_Init(void)
{
    // 1. Конфігурація PWM (швидкість)
    HWD_PWM_Config_t pwm_config = {
        .frequency = 1000,              // 1 kHz
        .resolution = 999,              // 1000 кроків (0.1% точність)
        .channel = HWD_PWM_CHANNEL_1,
        .hw_handle = &htim3,
        .hw_channel = TIM_CHANNEL_1
    };
    HWD_PWM_Init(&pwm_handle, &pwm_config);

    // 2. Конфігурація драйвера (PWM + DIR режим)
    PWM_Motor_Config_t motor_config = {
        .type = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,  // PWM + DIR режим
        .pwm_fwd = &pwm_handle,                 // PWM для швидкості
        .pwm_bwd = NULL,                        // Не використовується
        .gpio_dir = GPIOA,                      // Порт для DIR
        .gpio_pin = GPIO_PIN_0                  // Пін PA0 для DIR
    };
    PWM_Motor_Create(&motor, &motor_config);

    // 3. Параметри мотора
    Motor_Params_t params = {
        .type = MOTOR_TYPE_DC_BRUSHED,
        .max_power = 100.0f,
        .min_power = 10.0f,
        .max_current = 2.0f,
        .max_temperature = 80.0f,
        .max_rpm = 6000,
        .invert_direction = false   // false: HIGH=вперед, true: LOW=вперед
    };
    PWM_Motor_Init(&motor, &params);
}

/**
 * @brief Простий тест мотора
 */
void Motor_Simple_Test(void)
{
    // Рух вперед 50%
    PWM_Motor_SetPower(&motor, 50.0f);
    HAL_Delay(2000);

    // Зупинка
    PWM_Motor_Stop(&motor);
    HAL_Delay(500);

    // Рух назад 30%
    PWM_Motor_SetPower(&motor, -30.0f);
    HAL_Delay(2000);

    // Зупинка
    PWM_Motor_Stop(&motor);
}
```

## Приклади тестування

### 1. Тест плавного розгону

```c
void Motor_Test_Ramp(void)
{
    // Розгін від 0% до 100%
    for (float power = 0.0f; power <= 100.0f; power += 5.0f) {
        PWM_Motor_SetPower(&motor, power);
        HAL_Delay(100);  // 100 мс на крок
    }

    // Гальмування від 100% до 0%
    for (float power = 100.0f; power >= 0.0f; power -= 5.0f) {
        PWM_Motor_SetPower(&motor, power);
        HAL_Delay(100);
    }

    PWM_Motor_Stop(&motor);
}
```

### 2. Тест зміни напрямку

```c
void Motor_Test_Direction(void)
{
    // Вперед
    PWM_Motor_SetPower(&motor, 40.0f);
    HAL_Delay(2000);

    // ВАЖЛИВО: Зупинка перед зміною напрямку!
    PWM_Motor_Stop(&motor);
    HAL_Delay(200);  // Пауза для механічної зупинки

    // Назад
    PWM_Motor_SetPower(&motor, -40.0f);
    HAL_Delay(2000);

    PWM_Motor_Stop(&motor);
}
```

### 3. Тест з контролем стану

```c
void Motor_Test_With_Status(void)
{
    Motor_Stats_t stats;
    Motor_State_t state;

    // Запуск мотора
    PWM_Motor_SetPower(&motor, 60.0f);
    HAL_Delay(1000);

    // Отримання статистики
    PWM_Motor_GetStats(&motor, &stats);
    PWM_Motor_GetState(&motor, &state);

    // Перевірка стану (потребує UART для printf)
    printf("State: %d\n", state);
    printf("Power: %.1f%%\n", stats.current_power);
    printf("Direction: %d\n", stats.direction);
    printf("Run time: %lu ms\n", stats.run_time_ms);

    PWM_Motor_Stop(&motor);
}
```

## API Довідка

### Основні функції

#### Ініціалізація

```c
Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                const PWM_Motor_Config_t* config);
```
Створює драйвер з конфігурацією PWM+DIR.

```c
Servo_Status_t PWM_Motor_Init(PWM_Motor_Driver_t* driver,
                              const Motor_Params_t* params);
```
Ініціалізує драйвер з параметрами мотора.

#### Керування

```c
Servo_Status_t PWM_Motor_SetPower(PWM_Motor_Driver_t* driver, float power);
```
Встановлює потужність мотора.
- **power**: від -100.0 до +100.0
  - Додатні значення: обертання вперед (DIR = HIGH)
  - Від'ємні значення: обертання назад (DIR = LOW)
  - 0: зупинка

```c
Servo_Status_t PWM_Motor_Stop(PWM_Motor_Driver_t* driver);
```
Зупиняє мотор (PWM = 0).

```c
Servo_Status_t PWM_Motor_EmergencyStop(PWM_Motor_Driver_t* driver);
```
Аварійна зупинка (негайна).

#### Отримання інформації

```c
Servo_Status_t PWM_Motor_GetState(PWM_Motor_Driver_t* driver,
                                  Motor_State_t* state);
```
Отримує поточний стан мотора.

```c
Servo_Status_t PWM_Motor_GetStats(PWM_Motor_Driver_t* driver,
                                  Motor_Stats_t* stats);
```
Отримує статистику роботи мотора.

## Налаштування параметрів

### PWM_Motor_Config_t

```c
typedef struct {
    PWM_Motor_Type_t type;       // PWM_MOTOR_TYPE_SINGLE_PWM_DIR
    HWD_PWM_Handle_t* pwm_fwd;   // PWM канал для швидкості
    HWD_PWM_Handle_t* pwm_bwd;   // NULL для PWM+DIR режиму
    void* gpio_dir;              // GPIO порт для DIR (GPIOA, GPIOB, ...)
    uint32_t gpio_pin;           // GPIO пін (GPIO_PIN_0, GPIO_PIN_1, ...)
} PWM_Motor_Config_t;
```

### Motor_Params_t

```c
typedef struct {
    Motor_Type_t type;           // MOTOR_TYPE_DC_BRUSHED
    float max_power;             // Максимальна потужність (0.0 - 100.0)
    float min_power;             // Мінімальна потужність для старту
    float max_current;           // Максимальний струм (A)
    float max_temperature;       // Максимальна температура (°C)
    uint32_t max_rpm;            // Максимальна швидкість (об/хв)
    bool invert_direction;       // Інверсія напрямку
} Motor_Params_t;
```

### Параметр invert_direction

| Значення | DIR = LOW | DIR = HIGH |
|----------|-----------|------------|
| `false` (за замовчуванням) | Назад | Вперед |
| `true` (інвертовано) | Вперед | Назад |

Використовуйте `true` якщо мотор обертається в протилежний бік.

## Налагодження

### Перевірка PWM сигналу

Використайте осцилоскоп або логічний аналізатор:

**PA6 (PWM):**
- Частота: 1 kHz (період 1 мс)
- Напруга: 0 - 3.3V
- Duty cycle: 0% - 100% (залежно від встановленої потужності)

**PA0 (DIR):**
- HIGH (3.3V): мотор обертається вперед
- LOW (0V): мотор обертається назад

### Перевірка через LED

Додайте індикацію:

```c
void Motor_Test_With_LED(void)
{
    // Включити LED перед запуском
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    PWM_Motor_SetPower(&motor, 50.0f);
    HAL_Delay(2000);

    PWM_Motor_Stop(&motor);

    // Вимкнути LED після зупинки
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}
```

### Типові проблеми та рішення

| Проблема | Можлива причина | Рішення |
|----------|-----------------|---------|
| Мотор не крутиться | PWM не запущений | Перевірте `MX_TIM3_Init()` в main.c |
| Мотор не крутиться | Недостатньо живлення | Перевірте джерело живлення мотора (6-12V) |
| Мотор крутить в інший бік | Неправильний напрямок | Встановіть `invert_direction = true` |
| Мотор не змінює напрямок | DIR не підключений | Перевірте з'єднання PA0 → DIR драйвера |
| Нестабільна робота | Спільна земля не з'єднана | З'єднайте GND STM32 та GND джерела мотора |
| Мотор дуже повільний | `min_power` занадто низька | Збільште `min_power` до 15-20% |

## Обмеження безпеки

### ВАЖЛИВО:

1. **Завжди зупиняйте мотор перед зміною напрямку**
   ```c
   PWM_Motor_Stop(&motor);
   HAL_Delay(200);  // Пауза 200 мс
   PWM_Motor_SetPower(&motor, -power);
   ```

2. **Не використовуйте повну потужність без навантаження**
   - Максимум 50-60% для тестів без навантаження
   - Повна потужність тільки з прикріпленим навантаженням

3. **Перевіряйте струм споживання**
   - Використовуйте амперметр для контролю струму
   - Не перевищуйте `max_current` параметр

4. **Захист від перегріву**
   - Не запускайте на повну потужність довше 10-15 секунд
   - Додайте радіатор на драйвер мотора при необхідності

## Порівняння режимів керування

| Режим | Піни | Переваги | Недоліки |
|-------|------|----------|----------|
| **PWM + DIR** | 2 | Простота, економія пінів | Обмежений контроль гальмування |
| **Dual PWM** | 2 PWM | Точне гальмування, реверс без зупинки | Більше навантаження на таймери |
| **PWM + 2 GPIO** | 1 PWM + 2 GPIO | Підходить для L298N | Складніше підключення |

## Наступні кроки

Після успішного тестування базового драйвера, ви можете:

1. **Додати датчик положення (AS5600)**
   - Зворотний зв'язок по позиції
   - Підрахунок обертів

2. **Налаштувати PID контролер**
   - Точне позиційне керування
   - Стабілізація швидкості

3. **Інтегрувати електронні гальма**
   - Fail-safe захист
   - Утримання позиції

4. **Використовувати Servo Controller**
   - Автоматичне керування
   - Генератор траєкторій
   - Система безпеки

## Додаткові ресурси

- [ServoLib README](../README.md) - Загальний огляд бібліотеки
- [Technical Specifications](../technical_specifications.md) - Технічна специфікація
- [Structure](../structure.md) - Структура проекту
- [MOTOR_DRIVER_EXAMPLE.md](../MOTOR_DRIVER_EXAMPLE.md) - Інші приклади драйверів

## Підтримка

При виникненні проблем:
1. Перевірте підключення за схемою
2. Використайте осцилоскоп для діагностики PWM
3. Перевірте налаштування CubeMX
4. Зверніться до повного прикладу в `motor_pwm_dir_example.c`

---

**ServoCore Team** | 2025 | КПІ ім. Ігоря Сікорського
