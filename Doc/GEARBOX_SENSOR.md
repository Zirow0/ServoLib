# Gearbox Sensor Module - Документація

## Огляд

**Gearbox Sensor** - це модуль ServoLib, який забезпечує роботу з редукторами в сервосистемах. Модуль надає **два незалежні інтерфейси** для доступу до кута валу двигуна та вихідного валу після редуктора.

### Ключові можливості

- 🔄 **Dual-Interface**: окремі інтерфейси для валу двигуна та вихідного валу
- ⚙️ **Гнучка конфігурація**: підтримка датчика на валу двигуна або на виході
- 🎯 **Автоматичний перерахунок**: кути та швидкості обох валів
- 🔧 **Незалежне калібрування**: окремі нульові точки для кожного валу
- 📊 **Повна прозорість**: стандартний `Sensor_Interface_t` для інтеграції
- 🛠️ **Компенсація люфту**: підтримка backlash compensation

---

## Архітектура

### Шарова модель

```
┌──────────────────────────────────────────────────────────┐
│              Application Layer                           │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ┌─────────────────┐         ┌─────────────────┐        │
│  │  BLDC Driver    │         │  PID Controller │        │
│  │  (commutation)  │         │  (position)     │        │
│  └────────┬────────┘         └────────┬────────┘        │
│           │                           │                  │
│           │ motor_iface               │ output_iface     │
│           │                           │                  │
├───────────┼───────────────────────────┼──────────────────┤
│           │   Gearbox_Sensor Module  │                  │
│           │         (N:1)             │                  │
│           ▼                           ▼                  │
│  ┌─────────────────┐         ┌─────────────────┐        │
│  │ Motor Shaft     │         │ Output Shaft    │        │
│  │ Interface       │◄────────┤ Interface       │        │
│  │                 │  gear   │                 │        │
│  └────────┬────────┘  ratio  └─────────────────┘        │
│           │                                              │
│           │ base_sensor                                  │
│           ▼                                              │
├───────────┴──────────────────────────────────────────────┤
│           Physical Sensor (AS5600, AEAT-9922, etc.)      │
└──────────────────────────────────────────────────────────┘
```

### Основна ідея

Модуль обгортає фізичний датчик (наприклад, AS5600) і надає:

1. **Motor Shaft Interface** - для отримання кута валу двигуна
   - Використовується для комутації BLDC
   - Використовується для контролю швидкості двигуна

2. **Output Shaft Interface** - для отримання кута вихідного валу
   - Використовується для PID контролера позиції
   - Використовується для траєкторного керування

---

## API Довідка

### Типи даних

#### `Gearbox_Mount_Type_t`

Тип встановлення датчика:

```c
typedef enum {
    GEARBOX_MOUNT_MOTOR_SHAFT = 0,  /**< Датчик на валу двигуна */
    GEARBOX_MOUNT_OUTPUT_SHAFT = 1  /**< Датчик на вихідному валу */
} Gearbox_Mount_Type_t;
```

#### `Gearbox_Sensor_Config_t`

Конфігурація редуктора:

```c
typedef struct {
    Sensor_Interface_t* base_sensor;  /**< Базовий фізичний датчик */
    Gearbox_Mount_Type_t mount_type;  /**< Де встановлено датчик */
    float gear_ratio;                 /**< Передаточне відношення (N:1) */
    bool invert_motor_direction;      /**< Інверсія напрямку двигуна */
    bool invert_output_direction;     /**< Інверсія напрямку виходу */
    float backlash;                   /**< Люфт редуктора (градуси на виході) */
} Gearbox_Sensor_Config_t;
```

**Параметри:**
- `base_sensor` - вказівник на інтерфейс фізичного датчика (AS5600, AEAT-9922, тощо)
- `mount_type` - місце встановлення датчика
- `gear_ratio` - передаточне відношення. Приклад: `10.0` означає 10 обертів двигуна = 1 оберт виходу
- `invert_motor_direction` - інвертувати напрямок обертання валу двигуна
- `invert_output_direction` - інвертувати напрямок обертання вихідного валу
- `backlash` - величина люфту в градусах (на вихідному валу)

#### `Gearbox_Sensor_Handle_t`

Дескриптор датчика з редуктором:

```c
typedef struct {
    Gearbox_Sensor_Config_t config;   /**< Конфігурація */

    // === ДВА ПУБЛІЧНИХ ІНТЕРФЕЙСИ ===
    Sensor_Interface_t motor_shaft_interface;   /**< Інтерфейс валу двигуна */
    Sensor_Interface_t output_shaft_interface;  /**< Інтерфейс вихідного валу */

    // Внутрішні дані
    float motor_angle;                /**< Кут валу двигуна (градуси) */
    float motor_velocity;             /**< Швидкість валу двигуна (град/с) */
    float output_angle;               /**< Кут вихідного валу (градуси) */
    float output_velocity;            /**< Швидкість вихідного валу (град/с) */

    // Калібрування
    float motor_zero_offset;          /**< Зміщення нуля для двигуна */
    float output_zero_offset;         /**< Зміщення нуля для виходу */

    // Статистика
    uint32_t read_count;
    uint32_t error_count;
    bool data_valid;
} Gearbox_Sensor_Handle_t;
```

---

### Функції

#### `Gearbox_Sensor_Init()`

Ініціалізація датчика з редуктором.

```c
Servo_Status_t Gearbox_Sensor_Init(
    Gearbox_Sensor_Handle_t* handle,
    const Gearbox_Sensor_Config_t* config
);
```

**Параметри:**
- `handle` - вказівник на дескриптор
- `config` - вказівник на конфігурацію

**Повертає:** `SERVO_OK` при успіху

**Опис:**
Ініціалізує модуль та створює два незалежні інтерфейси:
- `handle->motor_shaft_interface` - для валу двигуна
- `handle->output_shaft_interface` - для вихідного валу

---

#### `Gearbox_Sensor_Update()`

Оновлення даних з датчика.

```c
Servo_Status_t Gearbox_Sensor_Update(Gearbox_Sensor_Handle_t* handle);
```

**Параметри:**
- `handle` - вказівник на дескриптор

**Повертає:** `SERVO_OK` при успіху

**Опис:**
Читає фізичний датчик і обчислює кути обох валів. **Має викликатися періодично** (наприклад, на частоті 1 kHz) в головному циклі.

---

#### `Gearbox_Sensor_GetMotorInterface()`

Отримання інтерфейсу валу двигуна.

```c
Sensor_Interface_t* Gearbox_Sensor_GetMotorInterface(
    Gearbox_Sensor_Handle_t* handle
);
```

**Параметри:**
- `handle` - вказівник на дескриптор

**Повертає:** вказівник на `Sensor_Interface_t` для валу двигуна

**Використання:**
```c
// Передати BLDC драйверу для комутації
bldc_config.commutation_sensor = Gearbox_Sensor_GetMotorInterface(&gearbox);
```

---

#### `Gearbox_Sensor_GetOutputInterface()`

Отримання інтерфейсу вихідного валу.

```c
Sensor_Interface_t* Gearbox_Sensor_GetOutputInterface(
    Gearbox_Sensor_Handle_t* handle
);
```

**Параметри:**
- `handle` - вказівник на дескриптор

**Повертає:** вказівник на `Sensor_Interface_t` для вихідного валу

**Використання:**
```c
// Передати PID контролеру для позиціонування
servo_config.position_sensor = Gearbox_Sensor_GetOutputInterface(&gearbox);
```

---

#### `Gearbox_Sensor_GetMotorAngle()`

Прямий доступ до кута валу двигуна.

```c
Servo_Status_t Gearbox_Sensor_GetMotorAngle(
    const Gearbox_Sensor_Handle_t* handle,
    float* angle
);
```

**Параметри:**
- `handle` - вказівник на дескриптор
- `angle` - вказівник для збереження кута (градуси)

**Повертає:** `SERVO_OK` при успіху

---

#### `Gearbox_Sensor_GetOutputAngle()`

Прямий доступ до кута вихідного валу.

```c
Servo_Status_t Gearbox_Sensor_GetOutputAngle(
    const Gearbox_Sensor_Handle_t* handle,
    float* angle
);
```

**Параметри:**
- `handle` - вказівник на дескриптор
- `angle` - вказівник для збереження кута (градуси)

**Повертає:** `SERVO_OK` при успіху

---

#### `Gearbox_Sensor_SetMotorZero()`

Встановлення нульової точки для валу двигуна.

```c
Servo_Status_t Gearbox_Sensor_SetMotorZero(Gearbox_Sensor_Handle_t* handle);
```

**Параметри:**
- `handle` - вказівник на дескриптор

**Повертає:** `SERVO_OK` при успіху

**Опис:**
Встановлює поточне положення валу двигуна як нульову точку. Корисно для калібрування комутації BLDC.

---

#### `Gearbox_Sensor_SetOutputZero()`

Встановлення нульової точки для вихідного валу.

```c
Servo_Status_t Gearbox_Sensor_SetOutputZero(Gearbox_Sensor_Handle_t* handle);
```

**Параметри:**
- `handle` - вказівник на дескриптор

**Повертає:** `SERVO_OK` при успіху

**Опис:**
Встановлює поточне положення вихідного валу як нульову точку. Корисно для калібрування домашньої позиції.

---

#### `Gearbox_Sensor_SetGearRatio()`

Зміна передаточного відношення.

```c
Servo_Status_t Gearbox_Sensor_SetGearRatio(
    Gearbox_Sensor_Handle_t* handle,
    float gear_ratio
);
```

**Параметри:**
- `handle` - вказівник на дескриптор
- `gear_ratio` - нове передаточне відношення

**Повертає:** `SERVO_OK` при успіху

**Опис:**
Дозволяє змінити передаточне відношення під час роботи. Корисно для систем зі змінним редуктором.

---

## Математика перерахунку

### Випадок 1: Датчик на валу двигуна

```c
mount_type = GEARBOX_MOUNT_MOTOR_SHAFT
gear_ratio = 10.0  // 10 обертів двигуна = 1 оберт виходу
```

**Перерахунок:**
```c
// Читання з датчика
raw_angle = read_from_sensor();  // Кут валу двигуна

// Обчислення для валу двигуна
motor_angle = raw_angle - motor_zero_offset;
motor_velocity = raw_velocity;

// Обчислення для вихідного валу
output_angle = motor_angle / gear_ratio;
output_velocity = motor_velocity / gear_ratio;
```

**Приклад:**
- Датчик показує: `3600°` (10 обертів двигуна)
- Кут вихідного валу: `3600 / 10 = 360°` (1 оберт)

---

### Випадок 2: Датчик на вихідному валу

```c
mount_type = GEARBOX_MOUNT_OUTPUT_SHAFT
gear_ratio = 10.0
```

**Перерахунок:**
```c
// Читання з датчика
raw_angle = read_from_sensor();  // Кут вихідного валу

// Обчислення для вихідного валу
output_angle = raw_angle - output_zero_offset;
output_velocity = raw_velocity;

// Обчислення для валу двигуна
motor_angle = output_angle * gear_ratio;
motor_velocity = output_velocity * gear_ratio;
```

**Приклад:**
- Датчик показує: `36°` (0.1 оберта виходу)
- Кут валу двигуна: `36 * 10 = 360°` (1 оберт двигуна)

---

## Приклади використання

### Приклад 1: DC двигун з редуктором 10:1

```c
// ═══════════════════════════════════════════════════════════
// Базова конфігурація
// ═══════════════════════════════════════════════════════════

// 1. Ініціалізація фізичного датчика AS5600 на валу двигуна
AS5600_Driver_t as5600;
AS5600_Config_t as5600_config = {
    .i2c = &i2c_handle,
    .address = AS5600_I2C_ADDRESS,
    .use_raw_angle = false
};
AS5600_Create(&as5600, &as5600_config);

Sensor_Params_t sensor_params = {
    .type = SENSOR_TYPE_ENCODER_MAG,
    .resolution = 4096,
    .update_rate = 1000
};
AS5600_Init(&as5600, &sensor_params);

// 2. Створення gearbox модуля
Gearbox_Sensor_Handle_t gearbox;
Gearbox_Sensor_Config_t gearbox_config = {
    .base_sensor = &as5600.interface,
    .mount_type = GEARBOX_MOUNT_MOTOR_SHAFT,  // Датчик на двигуні
    .gear_ratio = 10.0f,                      // Редуктор 10:1
    .invert_motor_direction = false,
    .invert_output_direction = false,
    .backlash = 0.5f                          // 0.5° люфт
};
Gearbox_Sensor_Init(&gearbox, &gearbox_config);

// 3. Підключення до PID контролера (вихідний вал)
Servo_Controller_t servo;
Servo_Config_t servo_config = {
    .update_frequency = 1000.0f,
    // ... інші параметри PID ...
};
Servo_Init(&servo, &servo_config,
           &motor.interface,
           Gearbox_Sensor_GetOutputInterface(&gearbox));  // ← Вихідний вал

// ═══════════════════════════════════════════════════════════
// Головний цикл
// ═══════════════════════════════════════════════════════════
while (1) {
    // Оновити gearbox (читає датчик + обчислює обидва вали)
    Gearbox_Sensor_Update(&gearbox);

    // Оновити контролер (отримує кут вихідного валу автоматично)
    Servo_Update(&servo);

    // Встановити позицію у градусах ВИХІДНОГО валу
    Servo_SetPosition(&servo, 90.0f);  // 90° на виході

    HAL_Delay(1);  // 1 kHz
}
```

---

### Приклад 2: BLDC двигун з редуктором 50:1

```c
// ═══════════════════════════════════════════════════════════
// BLDC + редуктор + датчик на двигуні
// ═══════════════════════════════════════════════════════════

// 1. Фізичний датчик AEAT-9922 (високороздільний)
AEAT9922_Driver_t aeat9922;
AEAT9922_Config_t aeat_config = {
    .spi = &spi_handle,
    .cs_port = GPIOA,
    .cs_pin = GPIO_PIN_4,
    .resolution = AEAT9922_RESOLUTION_16BIT
};
AEAT9922_Create(&aeat9922, &aeat_config);
AEAT9922_Init(&aeat9922, &sensor_params);

// 2. Gearbox модуль
Gearbox_Sensor_Handle_t gearbox;
Gearbox_Sensor_Config_t gearbox_config = {
    .base_sensor = &aeat9922.interface,
    .mount_type = GEARBOX_MOUNT_MOTOR_SHAFT,
    .gear_ratio = 50.0f,              // Редуктор 50:1
    .invert_motor_direction = false,
    .invert_output_direction = true,  // Інвертувати вихід
    .backlash = 1.0f
};
Gearbox_Sensor_Init(&gearbox, &gearbox_config);

// 3. BLDC драйвер (потребує кут валу двигуна)
BLDC_Driver_t bldc;
BLDC_Config_t bldc_config = {
    .commutation_sensor = Gearbox_Sensor_GetMotorInterface(&gearbox),  // ← Вал двигуна!
    .pole_pairs = 7,
    .pwm_frequency = 20000
};
BLDC_Init(&bldc, &bldc_config);

// 4. PID контролер позиції (працює з вихідним валом)
Servo_Controller_t servo;
Servo_Config_t servo_config = {
    .update_frequency = 1000.0f,
    // ... PID параметри ...
};
Servo_Init(&servo, &servo_config,
           &bldc.motor_interface,
           Gearbox_Sensor_GetOutputInterface(&gearbox));   // ← Вихідний вал!

// ═══════════════════════════════════════════════════════════
// Головний цикл
// ═══════════════════════════════════════════════════════════
while (1) {
    // Оновити gearbox
    Gearbox_Sensor_Update(&gearbox);

    // BLDC автоматично отримує кут валу двигуна (для комутації)
    BLDC_Update(&bldc);

    // PID автоматично отримує кут вихідного валу (для позиції)
    Servo_Update(&servo);

    // Діагностика: можна читати обидва кути
    float motor_angle, output_angle;
    Gearbox_Sensor_GetMotorAngle(&gearbox, &motor_angle);
    Gearbox_Sensor_GetOutputAngle(&gearbox, &output_angle);

    printf("Motor: %.2f°, Output: %.2f°\n", motor_angle, output_angle);

    HAL_Delay(1);
}
```

---

### Приклад 3: Датчик на вихідному валу

```c
// ═══════════════════════════════════════════════════════════
// Конфігурація з датчиком на виході
// ═══════════════════════════════════════════════════════════

Gearbox_Sensor_Config_t gearbox_config = {
    .base_sensor = &as5600.interface,
    .mount_type = GEARBOX_MOUNT_OUTPUT_SHAFT,  // ← На виході!
    .gear_ratio = 10.0f,
    .invert_motor_direction = false,
    .invert_output_direction = false,
    .backlash = 0.2f
};
Gearbox_Sensor_Init(&gearbox, &gearbox_config);

// У цьому випадку:
// - Датчик читає кут вихідного валу безпосередньо
// - Модуль обчислює кут валу двигуна (множенням на gear_ratio)
// - BLDC все одно отримує коректний кут для комутації
```

---

### Приклад 4: Калібрування нульових точок

```c
// ═══════════════════════════════════════════════════════════
// Процедура калібрування
// ═══════════════════════════════════════════════════════════

void Calibrate_Gearbox(Gearbox_Sensor_Handle_t* gearbox)
{
    // 1. Встановити нуль валу двигуна (для BLDC комутації)
    printf("Встановіть вал двигуна в позицію 0° (фаза A)\n");
    printf("Натисніть Enter...\n");
    wait_for_enter();

    Gearbox_Sensor_SetMotorZero(gearbox);
    printf("✓ Нуль валу двигуна встановлено\n");

    // 2. Встановити нуль вихідного валу (домашня позиція)
    printf("Встановіть вихідний вал в домашню позицію\n");
    printf("Натисніть Enter...\n");
    wait_for_enter();

    Gearbox_Sensor_SetOutputZero(gearbox);
    printf("✓ Нуль вихідного валу встановлено\n");

    // 3. Перевірка
    float motor_angle, output_angle;
    Gearbox_Sensor_Update(gearbox);
    Gearbox_Sensor_GetMotorAngle(gearbox, &motor_angle);
    Gearbox_Sensor_GetOutputAngle(gearbox, &output_angle);

    printf("Поточні кути:\n");
    printf("  Вал двигуна: %.2f°\n", motor_angle);
    printf("  Вихідний вал: %.2f°\n", output_angle);
}
```

---

## Діагностика та налагодження

### Перевірка передаточного відношення

```c
void Test_Gear_Ratio(Gearbox_Sensor_Handle_t* gearbox, float expected_ratio)
{
    printf("Тест передаточного відношення...\n");

    float initial_motor, initial_output;
    Gearbox_Sensor_GetMotorAngle(gearbox, &initial_motor);
    Gearbox_Sensor_GetOutputAngle(gearbox, &initial_output);

    printf("Поверніть вихідний вал на 360° (1 оберт)\n");
    printf("Натисніть Enter коли готово...\n");
    wait_for_enter();

    float final_motor, final_output;
    Gearbox_Sensor_Update(gearbox);
    Gearbox_Sensor_GetMotorAngle(gearbox, &final_motor);
    Gearbox_Sensor_GetOutputAngle(gearbox, &final_output);

    float motor_delta = final_motor - initial_motor;
    float output_delta = final_output - initial_output;
    float measured_ratio = motor_delta / output_delta;

    printf("Результати:\n");
    printf("  Δ двигун: %.2f°\n", motor_delta);
    printf("  Δ вихід: %.2f°\n", output_delta);
    printf("  Виміряне відношення: %.2f:1\n", measured_ratio);
    printf("  Очікуване відношення: %.2f:1\n", expected_ratio);
    printf("  Похибка: %.2f%%\n",
           fabsf(measured_ratio - expected_ratio) / expected_ratio * 100.0f);
}
```

---

### Моніторинг у реальному часі

```c
void Monitor_Gearbox(Gearbox_Sensor_Handle_t* gearbox)
{
    while (1) {
        Gearbox_Sensor_Update(gearbox);

        float motor_angle, motor_vel;
        float output_angle, output_vel;

        Gearbox_Sensor_GetMotorAngle(gearbox, &motor_angle);
        Gearbox_Sensor_GetMotorVelocity(gearbox, &motor_vel);
        Gearbox_Sensor_GetOutputAngle(gearbox, &output_angle);
        Gearbox_Sensor_GetOutputVelocity(gearbox, &output_vel);

        printf("\rMotor: %7.2f° (%6.1f °/s) | Output: %7.2f° (%6.1f °/s) | Ratio: %.2f",
               motor_angle, motor_vel, output_angle, output_vel,
               motor_angle / output_angle);
        fflush(stdout);

        HAL_Delay(100);  // 10 Hz оновлення дисплею
    }
}
```

---

## Продуктивність

### Обчислювальна складність

**Операції на один виклик `Gearbox_Sensor_Update()`:**
- 1x читання фізичного датчика
- 2x віднімання (zero offset)
- 2x множення/ділення (gear ratio)
- Інверсія напрямку (опціонально)

**Час виконання:** < 50 мкс на STM32F411 @ 100 MHz

### Рекомендації

- Викликати `Gearbox_Sensor_Update()` на частоті 1-10 kHz
- Для BLDC комутації рекомендується ≥ 10 kHz
- Для PID контролера достатньо 1 kHz

---

## Обмеження

### Поточна версія

- ✅ Підтримка одноступінчастого редуктора
- ✅ Базова компенсація люфту (заплановано)
- ❌ Багатоступінчасті редуктори (заплановано в v2.0)
- ❌ Пружність валу (compliance) - заплановано

### Точність

**Точність перерахунку:**
- Обмежена роздільністю фізичного датчика
- Для AS5600 (12-біт): 0.088° на оберт
- Для AEAT-9922 (16-біт): 0.0055° на оберт

**Вплив редуктора на точність вихідного валу:**
- При `gear_ratio = 10`: точність покращується в 10 разів
- При `gear_ratio = 50`: точність покращується в 50 разів

---

## FAQ

### Питання 1: Чи можна використовувати без редуктора?

**Відповідь:** Так, встановіть `gear_ratio = 1.0`. У цьому випадку обидва інтерфейси повертатимуть однакові значення.

---

### Питання 2: Чи можна змінити gear_ratio під час роботи?

**Відповідь:** Так, викличте `Gearbox_Sensor_SetGearRatio()`. Корисно для систем зі змінним редуктором або для налагодження.

---

### Питання 3: Як працює з багатообертовими енкодерами?

**Відповідь:** Модуль працює з кутами в градусах. Для багатообертових енкодерів:
- Кут може перевищувати 360°
- Наприклад: 3600° = 10 обертів
- Додаток відповідає за обробку багатообертовості

---

### Питання 4: Чи підтримується зворотна кінематика?

**Відповідь:** Так, модуль автоматично обчислює кути обох валів незалежно від місця встановлення датчика.

---

### Питання 5: Як обробляється люфт (backlash)?

**Відповідь:** У поточній версії параметр `backlash` зберігається, але активна компенсація буде додана в наступній версії. Заплановано:
- Виявлення зміни напрямку
- Застосування компенсації люфту
- Згладжування переходів

---

## Підтримка

### Повідомлення про помилки

Якщо виявили помилку або маєте пропозиції:
1. Перевірте поточну версію модуля
2. Створіть issue в репозиторії
3. Надайте код для відтворення проблеми

### Додаткова документація

- [README.md](README.md) - Загальний огляд ServoLib
- [MOTOR_DRIVER_EXAMPLE.md](MOTOR_DRIVER_EXAMPLE.md) - Приклади драйвера мотора
- [technical_specifications.md](technical_specifications.md) - Технічна специфікація

---

## Ліцензія

MIT License

Copyright (c) 2025 ServoCore Team

---

**Версія документу:** 1.0
**Дата:** 2025-01-18
**Автор:** ServoCore Team
