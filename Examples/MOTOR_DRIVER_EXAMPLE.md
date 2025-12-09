# Приклад використання драйвера двигуна

> **ПРИМІТКА**: Цей приклад використовує застарілу архітектуру HAL. В поточній версії проекту використовується HWD (Hardware Driver Layer). Приклади коду в цьому файлі потребують оновлення.

## Огляд

Цей документ показує як використовувати PWM драйвер двигуна в проекті ServoCore.

## Структура драйвера

```
ServoLib/
├── Inc/
│   ├── core.h              # Основні типи даних
│   ├── config.h            # Конфігурація
│   ├── hal/
│   │   └── hal_pwm.h       # HAL абстракція PWM
│   ├── iface/
│   │   └── motor.h         # Інтерфейс двигуна
│   └── drv/
│       └── motor/
│           ├── base.h      # Базовий драйвер
│           └── pwm.h       # PWM драйвер
└── Board/STM32F411/
    ├── board_config.h      # Конфігурація плати
    └── hal_pwm.c           # Реалізація HAL PWM
```

## Базовий приклад

### 1. Включення необхідних заголовків

```c
#include "drv/motor/pwm.h"
#include "Board/STM32F411/board_config.h"
```

### 2. Оголошення змінних

```c
// HAL PWM дескриптори
HAL_PWM_Handle_t pwm_fwd_handle;
HAL_PWM_Handle_t pwm_bwd_handle;

// PWM драйвер двигуна
PWM_Motor_Driver_t motor_driver;
```

### 3. Ініціалізація PWM каналів

```c
void Motor_PWM_Init(void)
{
    // Конфігурація прямого каналу PWM
    HAL_PWM_Config_t pwm_fwd_config = {
        .frequency = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel = HAL_PWM_CHANNEL_1,
        .hw_handle = &MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_CHANNEL_FWD
    };

    // Конфігурація зворотного каналу PWM
    HAL_PWM_Config_t pwm_bwd_config = {
        .frequency = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel = HAL_PWM_CHANNEL_2,
        .hw_handle = &MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_CHANNEL_BWD
    };

    // Ініціалізація HAL PWM
    HAL_PWM_Init(&pwm_fwd_handle, &pwm_fwd_config);
    HAL_PWM_Init(&pwm_bwd_handle, &pwm_bwd_config);
}
```

### 4. Створення та ініціалізація драйвера

```c
void Motor_Driver_Init(void)
{
    // Конфігурація PWM драйвера двигуна
    PWM_Motor_Config_t motor_config = {
        .type = PWM_MOTOR_TYPE_DUAL_PWM,  // Два PWM канали (H-bridge)
        .pwm_fwd = &pwm_fwd_handle,
        .pwm_bwd = &pwm_bwd_handle,
        .gpio_dir = NULL,                 // Не використовується для dual PWM
        .gpio_pin = 0
    };

    // Створення драйвера
    PWM_Motor_Create(&motor_driver, &motor_config);

    // Параметри двигуна
    Motor_Params_t motor_params = {
        .max_power = 100.0f,        // Максимальна потужність 100%
        .min_power = 5.0f,          // Мінімальна потужність для старту 5%
        .max_current = 2000,        // Максимальний струм 2000 mA
        .max_rpm = 6000,            // Максимальна швидкість 6000 об/хв
        .invert_direction = false   // Не інвертувати напрямок
    };

    // Ініціалізація драйвера
    PWM_Motor_Init(&motor_driver, &motor_params);
}
```

### 5. Використання драйвера

```c
void Motor_Control_Example(void)
{
    // Встановлення потужності 50% вперед
    PWM_Motor_SetPower(&motor_driver, 50.0f);
    HAL_Delay(2000);  // Працюємо 2 секунди

    // Зупинка
    PWM_Motor_Stop(&motor_driver);
    HAL_Delay(500);

    // Встановлення потужності 30% назад
    PWM_Motor_SetPower(&motor_driver, -30.0f);
    HAL_Delay(2000);

    // Зупинка
    PWM_Motor_Stop(&motor_driver);
}
```

### 6. Отримання статистики

```c
void Motor_Print_Stats(void)
{
    Motor_Stats_t stats;
    Motor_State_t state;

    // Отримання статистики
    PWM_Motor_GetStats(&motor_driver, &stats);
    PWM_Motor_GetState(&motor_driver, &state);

    // Виведення інформації (потрібен UART)
    printf("Motor State: %d\n", state);
    printf("Run Time: %lu ms\n", stats.run_time_ms);
    printf("Total Starts: %lu\n", stats.total_starts);
    printf("Current Power: %.2f%%\n", stats.current_power);
    printf("Direction: %d\n", stats.direction);
}
```

## Повний приклад для main.c

```c
/* Includes */
#include "main.h"
#include "drv/motor/pwm.h"
#include "Board/STM32F411/board_config.h"

/* Private variables */
HAL_PWM_Handle_t pwm_fwd_handle;
HAL_PWM_Handle_t pwm_bwd_handle;
PWM_Motor_Driver_t motor_driver;

/* Private function prototypes */
void Motor_System_Init(void);
void Motor_Control_Task(void);

int main(void)
{
    /* MCU Configuration */
    HAL_Init();
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_I2C1_Init();
    MX_TIM5_Init();

    /* USER CODE BEGIN 2 */

    // Ініціалізація моторної системи
    Motor_System_Init();

    /* USER CODE END 2 */

    /* Infinite loop */
    while (1)
    {
        /* USER CODE BEGIN 3 */

        // Виконання задач керування мотором
        Motor_Control_Task();

        HAL_Delay(10);  // 100 Hz цикл оновлення

        /* USER CODE END 3 */
    }
}

/* Private functions */

void Motor_System_Init(void)
{
    // 1. Ініціалізація PWM каналів
    HAL_PWM_Config_t pwm_fwd_config = {
        .frequency = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel = HAL_PWM_CHANNEL_1,
        .hw_handle = &MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_CHANNEL_FWD
    };

    HAL_PWM_Config_t pwm_bwd_config = {
        .frequency = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel = HAL_PWM_CHANNEL_2,
        .hw_handle = &MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_CHANNEL_BWD
    };

    HAL_PWM_Init(&pwm_fwd_handle, &pwm_fwd_config);
    HAL_PWM_Init(&pwm_bwd_handle, &pwm_bwd_config);

    // 2. Створення драйвера мотора
    PWM_Motor_Config_t motor_config = {
        .type = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd = &pwm_fwd_handle,
        .pwm_bwd = &pwm_bwd_handle,
        .gpio_dir = NULL,
        .gpio_pin = 0
    };

    PWM_Motor_Create(&motor_driver, &motor_config);

    // 3. Ініціалізація параметрів мотора
    Motor_Params_t motor_params = {
        .max_power = 100.0f,
        .min_power = 5.0f,
        .max_current = 2000,
        .max_rpm = 6000,
        .invert_direction = false
    };

    PWM_Motor_Init(&motor_driver, &motor_params);
}

void Motor_Control_Task(void)
{
    static uint32_t last_time = 0;
    static uint8_t state = 0;

    uint32_t current_time = HAL_GetTick();

    // Змінюємо стан кожні 2 секунди
    if (current_time - last_time >= 2000) {
        last_time = current_time;

        switch (state) {
            case 0:
                // Прямий хід 50%
                PWM_Motor_SetPower(&motor_driver, 50.0f);
                state = 1;
                break;

            case 1:
                // Зупинка
                PWM_Motor_Stop(&motor_driver);
                state = 2;
                break;

            case 2:
                // Зворотний хід 30%
                PWM_Motor_SetPower(&motor_driver, -30.0f);
                state = 3;
                break;

            case 3:
                // Зупинка
                PWM_Motor_Stop(&motor_driver);
                state = 0;
                break;
        }
    }

    // Періодичне оновлення драйвера
    PWM_Motor_Update(&motor_driver);
}
```

## Додавання файлів до проекту STM32CubeIDE

### 1. Додавання шляхів включення

В Project Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths:

```
../ServoLib/Inc
../ServoLib/Board/STM32F411
```

### 2. Додавання вихідних файлів

Додайте до проекту такі файли:

```
ServoLib/Src/iface/motor.c
ServoLib/Src/drv/motor/base.c
ServoLib/Src/drv/motor/pwm.c
ServoLib/Board/STM32F411/hal_pwm.c
ServoLib/Board/STM32F411/board.c
```

## Налагодження

### Можливі проблеми

1. **Мотор не обертається**
   - Перевірте чи запущені PWM таймери (TIM3)
   - Перевірте підключення H-bridge драйвера
   - Перевірте живлення мотора

2. **Мотор обертається не в той бік**
   - Встановіть `invert_direction = true` в параметрах мотора
   - Або поміняйте місцями pwm_fwd і pwm_bwd

3. **Помилки компіляції**
   - Переконайтеся що всі шляхи включення додані правильно
   - Перевірте що STM32F4 HAL драйвери увімкнені

## Наступні кроки

1. Інтеграція з датчиком положення AS5600
2. Налаштування параметрів PID регулятора
3. Тестування плавного керування
4. Налаштування параметрів безпеки

## Додаткова інформація

Детальну документацію дивіться в:
- `technical_cpecifications.md` - технічні вимоги
- `structure.md` - структура бібліотеки
- `CUBEMX_SETUP.md` - налаштування CubeMX
