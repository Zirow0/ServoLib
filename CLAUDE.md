# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

**ServoLib** is a modular, portable C library for controlling DC servo drives on STM32F4 platforms using **libopencm3** (not STM32 HAL). Built on a 5-layer architecture with complete hardware abstraction.

**Одиниці вимірювання:** кут — **радіани**, швидкість — **рад/с** скрізь у API (ctrl, drv, servo). Конвертація в градуси — лише в Apps для UART-виводу.

## Build Commands

**Prerequisites:** Set `LIBOPENCM3_DIR` before configuring:
```bash
export LIBOPENCM3_DIR=/path/to/libopencm3
```

**Build targets** (`Apps/`): `debug_encoder`, `debug_motor`, `debug_brake`, `debug_current`, `servo_full`, `servo_basic`

```bash
# Configure (інтерактивний вибір плати, цілі та програматора):
./configure.sh

# Build:
./build.sh

# Flash:
./flash.sh
# або через CMake:
cmake --build build/<BOARD>/<APP> --target flash
```

`configure.sh` автоматично виявляє доступні плати з `cmake/targets/*.cmake` і цілі з `Apps/*/`. Стан зберігається у `.preset` (sourceable bash): `BOARD`, `APP`, `PROGRAMMER` (тип), `PROGRAMMER_SERIAL` (ID пристрою). `build.sh` і `flash.sh` читають `.preset`.

`flash.sh`: якщо `PROGRAMMER_SERIAL` збережено — використовує одразу без сканування. Якщо тип збережено без ID — сканує sysfs, при кількох пристроях показує меню `Назва (ID: ...)`. Якщо програматор не обрано — питає тип при кожному виклику.

**CMake layer:** `cmake/stm32.cmake` містить спільну логіку для всіх плат: `genlink.py` автовизначає CPU/FPU/DEFS/FAMILY з `devices.data`, лінкер-скрипт генерується автоматично з `OCM3/ld/linker.ld.S`. Файл плати (`cmake/targets/<BOARD>.cmake`) містить лише `DEVICE` і `BOARD_SRCS`.

**No unit test framework exists.** Testing is done on real hardware (STM32F411CEU6 BlackPill) via UART output from the debug apps.

## Architecture

```
Apps/                          ← Debug/application targets (main.c per target)
    ↓
Src/ctrl/                      ← Cascade PID, Safety, Trajectory, Servo coordinator
Src/comm/                      ← Async comm шар (hardware-independent)
    ↓
Src/drv/                       ← Motor, Position sensor, Brake, Current drivers
    ↓
Inc/hwd/ + Board/              ← HWD declarations + libopencm3 implementations
    STM32F411_OCM3/            ← основна плата (BlackPill)
    STM32F411_EncoderHub/      ← плата з кількома енкодерами
Lib/frame_codec/               ← COBS framing + CRC32 (git submodule)
Lib/packet_codec/              ← msg_id | payload layer (git submodule)
```

**Key principle:** `ctrl/`, `drv/`, `comm/` are fully hardware-independent. Only `Board/` imports libopencm3. Never call libopencm3 functions from `ctrl/`, `drv/` or `comm/`.

### Universal Interface Pattern

Всі чотири типи драйверів (Motor, Position, Brake, Current) використовують однаковий патерн:

1. **Universal interface struct** (`Motor_Interface_t`, `Position_Sensor_Interface_t`, `Brake_Interface_t`, `Current_Sensor_Interface_t`) — спільні дані + hw callback function pointers
2. **Specific driver struct** (напр. `PWM_Motor_Driver_t`, `ACS712_Driver_t`) — interface як **перше поле**, плюс апаратний стан
3. **Factory function** (напр. `PWM_Motor_Create()`, `ACS712_Create()`) — заповнює hw callbacks, викликає базовий Init
4. **Base functions** у `motor.c`/`position.c`/`brake.c`/`current.c` — спільна логіка, викликають callbacks для апаратних операцій

Доступ до universal interface через `&driver->interface`. Specific driver struct безпечно кастується до/від interface pointer бо interface — перше поле.

**Видалені з усіх драйверів:** `DeInit`/`deinit` callbacks, `is_initialized` поля.

### Servo Initialization

```c
// Повна ініціалізація з усіма компонентами:
Servo_InitFull(&servo, &config, &motor->interface, &sensor->interface,
               &brake->interface, &current->interface);

// Лише мотор (решта NULL):
Servo_Init(&servo, &config, &motor->interface);
```

`Servo_InitWithBrake()` **не існує** — використовувати `Servo_InitFull()`.
`Servo_InitFull` приймає **6 параметрів**: servo, config, motor, sensor, brake, current (усі крім motor — опціональні NULL).

### Режими керування (`Servo_Mode_t`)

| Режим | Активні контури | Функція |
|-------|----------------|---------|
| `SERVO_MODE_POSITION` | pos→vel→trq | `Servo_SetPosition(rad)` |
| `SERVO_MODE_VELOCITY` | vel→trq | `Servo_SetVelocity(rad/s)` |
| `SERVO_MODE_TORQUE` | trq | `Servo_SetTorque(A)` |
| `SERVO_MODE_IDLE` | — | `Servo_Stop()` |

## Configuration System

`Inc/config.h` містить:
- `POSITION_ERROR_THRESHOLD = 0.01f` рад (≈ 0.57°) — поріг `Servo_IsAtTarget()`

## Drivers

### Position Sensors

| Sensor | Resolution | Interface | Status |
|--------|-----------|-----------|--------|
| AS5600 | 12-bit (4096 cpr) | I2C IT continuous read | Available |
| Incremental | Quadrature EXTI X4 + IC timer | GPIO/TIM | Available |

AEAT-9922 видалено повністю.

**AS5600:** `HWD_I2C_StartContinuousRead()` запускає фоновий I2C IT цикл, дані постійно оновлюються у `volatile raw_buf[2]`. `HW_ReadRaw()` читає буфер миттєво.

**Incremental encoder:** Software EXTI X4 state machine → `volatile int32_t count` (32-bit, необмежений, підтримує 6+ датчиків). IC timer вимірює `volatile uint32_t period_us` для прямого розрахунку швидкості без диференціювання. Board ISR викликає:
```c
Incremental_Encoder_EXTI_Handler(driver, pin_a, pin_b);  // оновлює count
Incremental_Encoder_IC_Handler(driver, period_us);        // оновлює period_us
```

**Critical:** `HW_ReadRaw()` повертає `Position_Raw_Data_t.angle_rad` у **радіанах** — ніколи градуси. `position.c` нормалізує до `[0, 2π)` і рахує `velocity_rad_s`.

**Публічний API position (всі в радіанах):**
```c
Position_Sensor_GetPosition(sensor, &pos_rad);          // 0..2π рад
Position_Sensor_GetVelocity(sensor, &vel_rad_s);        // рад/с
Position_Sensor_GetAbsolutePosition(sensor, &abs_rad);  // необмежений рад
Position_Sensor_SetPosition(sensor, pos_rad);           // встановити нуль
```

`Position_Sensor_Init(sensor)` — multi-turn відстежується всередині кожного драйвера (інкрементальний через `count`, AS5600 через `revolution_count` у `HW_ReadRaw`).

### Motor Driver

PWM мотор підтримує два режими:
- `PWM_MOTOR_TYPE_SINGLE_PWM_DIR` — один PWM канал + DIR GPIO
- `PWM_MOTOR_TYPE_DUAL_PWM` — два PWM канали (H-bridge: L298N, TB6612)

`Motor_Params_t.max_power_rate` **не існує** — rate limiting прибрано. Slew rate знаходиться у `Cascade_Config_t.slew_rate`.

API: `Motor_Init`, `Motor_SetPower(motor, float power)`, `Motor_Stop`, `Motor_EmergencyStop`.
`Motor_Update()` не існує — оновлення відбувається всередині `Motor_SetPower`.

### Brake Driver

State machine: `ENGAGED ↔ ENGAGING ↔ RELEASING ↔ RELEASED`

Brake завжди ініціалізується у `ENGAGED` (fail-safe). Викликати `Brake_Update()` у control loop для обробки переходів.

API: `Brake_Init`, `Brake_Engage`, `Brake_Release`, `Brake_Update`, `Brake_GetState`, `Brake_IsEngaged`, `Brake_IsReleased`.
`Brake_DeInit`, `Brake_EmergencyEngage`, `Brake_IsTransitioning` — **не існують**. Для аварійної зупинки використовувати `Brake_Engage` напряму.

### Current Sensor Driver

**ACS712T** (ефект Хола, аналоговий вихід): варіанти 5A/20A/30A.

```c
// Ініціалізація:
HWD_ADC_Init(&adc, &adc_cfg);     // реєстрація каналу
HWD_ADC_StartScan();               // один раз після всіх ADC Init

const ACS712_Config_t acs_cfg = {  // тільки апаратні параметри
    .variant       = ACS712_30A,
    .adc           = &adc,
    .divider_ratio = 0.65f,
};
const Current_Params_t current_params = {
    .overcurrent_threshold_a = 4.0f,
    .process_noise_q         = 0.001f,  // дисперсія зміни струму за крок (А²)
    .measurement_noise_r     = 0.02f,   // дисперсія шуму датчика (А²) = σ²
};
ACS712_Create(&driver, &acs_cfg, &current_params);  // factory
Current_Sensor_Calibrate(&driver.interface);         // при нульовому струмі

// У control loop (оновлення UKF):
Current_Sensor_Update(&driver.interface);
// Servo_Update() сам читає струм через GetCurrent — не потрібно викликати у servo loop
Current_Sensor_GetCurrent(&driver.interface, &current_a);

// Додаткові функції:
Current_Sensor_GetPeakCurrent(&driver.interface, &peak_a);  // абсолютний пік з старту
Current_Sensor_IsOvercurrent(&driver.interface);             // true якщо перевантаження
Current_Sensor_ResetPeak(&driver.interface);                 // скинути пік та прапорець
```

`Current_Sensor_GetStats`, `Current_Sensor_DeInit` — **не існують**.

**Налаштування UKF фільтра:**
- `measurement_noise_r = σ²` де σ — виміряний шум датчика (А) при нульовому струмі
- `process_noise_q` — стартова точка: у 10-20x менше за `measurement_noise_r`
- Якщо фільтр запізнюється на стрибки — збільшити `process_noise_q`
- Якщо на виході ще є шум — збільшити `measurement_noise_r`

**CMake:** застосунки з `SERVOLIB_CURRENT` потребують `target_link_libraries(... ukf_mcu)`.

**Два порогові струми:**
- `Cascade_Config_t.vel.out_max` — робочий ліміт струму (А), обмежує вихід vel-контуру
- `Safety_Config_t.critical_current_a` — аварійний поріг, тригерить `EmergencyStop`

## Cascade PID Controller

Каскадний контролер (`Inc/ctrl/cascade.h`, `Src/ctrl/cascade.c`):

```
CASCADE_MODE_POS:  pos-PID(рад) → vel_sp(рад/с) → vel-PID → current_sp(А) → trq-PID → %
CASCADE_MODE_VEL:                  vel-PID(рад/с) → current_sp(А) → trq-PID → %
CASCADE_MODE_TRQ:                                    current_sp(А) → trq-PID → %
```

**Feedforward (спрощена модель):**
```c
ff = ff_j * vel_sp + ff_b * omega   // в %
// ff_j [%·с/рад], ff_b [%·с/рад]
// У CASCADE_MODE_TRQ feedforward не застосовується повністю
```

**Slew rate:** `config.slew_rate` [%/с] обмежує зміну команди двигуну. `0` = вимкнено.

**`Cascade_PID_Params_t`** для кожного контуру: `kp`, `ki`, `kd`, `out_min`, `out_max`, `i_limit`.
`i_limit > 0` — клемпує внесок інтегратора до `±i_limit` (в одиницях виходу).

**`PID_Params_t.i_limit`**: якщо `> 0` — незалежний ліміт інтегратора; `= 0` — старий механізм (clamp відносно `out_min/out_max - p_term`).

**`pid_mgr.c` не входить до `SERVOLIB_CTRL`** — замінено `cascade.c`. Файли `Inc/ctrl/pid_mgr.h` та `Src/ctrl/pid_mgr.c` залишені в репозиторії, але жодна ціль їх не компілює.

**`Cascade_ApplyConfig(casc, config)`** — оновлює PID коефіцієнти та ліміти без скидання інтеграторів. Безпечно викликати під час роботи для online тюнінгу.

## Safety

`Safety_Config_t` (всі одиниці в радіанах/рад·с):
- `min_position`, `max_position` — рад
- `max_velocity` — рад/с
- `max_acceleration` — рад/с²
- `critical_current_a` — А (аварійне вимкнення при перевищенні + таймаут)

## Async Comm Layer (`USE_COMM_ASYNC`)

Підключається через `target_compile_definitions(<app> PRIVATE USE_COMM_ASYNC)` у CMakeLists.txt застосунку. Реалізовано у `servo_basic`.

### Архітектура

```
frame_codec (COBS + CRC32/MPEG-2)
    └── packet_codec (msg_id | payload)
            └── servo_comm.c (encode/decode/dispatch)
                    └── hwd_uart_async.c (DMA TX queue + DMA RX circular + IDLE IRQ)
```

**ISR → main loop паттерн (критично для апаратного CRC32):**
```c
// ISR: тільки копіює байти у staging буфер, повертає bool (uart_rx_cb_t)
bool servo_comm_on_rx(const uint8_t *data, size_t len);

// Main loop: frame_decode + packet_decode (CRC32 тут — не в ISR)
void servo_comm_process_rx(void);
```

### Ініціалізація (порядок важливий)

```c
frame_crc32_init();                              // увімкнути RCC_CRC
servo_comm_init(comm_send);                      // реєструє send callback
uart_init(&comm_inst, &comm_hw, COMM_BAUD, servo_comm_on_rx);
servo_comm_seed_cascade_config(&initial_wire_cfg); // ініціалізувати буфер param-mode
```

`servo_comm_seed_cascade_config()` — потрібно викликати після `servo_comm_init()` з початковими значеннями конфігурації. Без цього param-mode оновлення (один коефіцієнт) повертатимуть структуру з нульовими значеннями в інших полях.

`uart_send()` є неблокуючим — копіює у TX pool (4 слоти), DMA передає у фоні.

### Структури протоколу

| Структура | Розмір | Напрям | msg_id |
|-----------|--------|--------|--------|
| `servo_telemetry_t` | 21 Б | STM32→хост | `0x02` struct / `0x03` param |
| `servo_command_t` | 5 Б | хост→STM32 | `0x04` struct / `0x05` param |
| `cascade_telemetry_t` | 81 Б | STM32→хост | `0x06` struct / `0x07` param |
| `cascade_config_t` | 84 Б | хост↔STM32 | `0x08` struct / `0x09` param |

**`cascade_telemetry_t`** — повний знімок каскадного PID: сенсори (θ, ω, I), сигнальний ланцюг (target→vel_sp→current_sp→ff→power), P/I/D терми для всіх трьох контурів. Поле `integral` (стан anti-windup) є лише у **vel** та **trq** контурів — у pos контуру `integral` в wire-структурі відсутній.

**`cascade_config_t`** — wire-формат налаштувань: kp/ki/kd/out_min/out_max/i_limit для pos, vel, trq + ff_j, ff_b, slew_rate. Підтримує struct mode (повна заміна) та param mode (один коефіцієнт).

### Single-param режим

Payload: `[offset 1B][type 1B][value N bytes]`

Константи індексів: `TELEM_FIELD_*`, `CASC_TELEM_*`, `CASC_CFG_*` у `Inc/comm/servo_protocol.h`.

### API

```c
// Ініціалізація
void servo_comm_init(servo_comm_send_fn send_fn);
void servo_comm_seed_cascade_config(const cascade_config_t *cfg);  // до uart_init

// TX
void servo_comm_send_telemetry(const servo_telemetry_t *);
void servo_comm_send_param(const servo_telemetry_t *, uint8_t field_idx);
void servo_comm_send_cascade(const cascade_telemetry_t *);
void servo_comm_send_cascade_param(const cascade_telemetry_t *, uint8_t field_idx);
void servo_comm_send_cascade_config(const cascade_config_t *);

// RX (викликати з main loop після servo_comm_process_rx)
bool servo_comm_get_command(servo_command_t *cmd_out);
bool servo_comm_get_cascade_config(cascade_config_t *cfg_out);
```

### Апаратний CRC32

`Board/STM32F411_OCM3/hwd_crc32.c` перевизначає `weak frame_crc32` з `crc32_soft.c`. Потребує `frame_crc32_init()` один раз при старті. Апаратний блок CRC — єдиний глобальний ресурс: не викликати одночасно з ISR та main loop (гарантується паттерном staging буфера).

### Конфігурація у `board_config.h`

```c
#ifdef USE_COMM_ASYNC
#define COMM_USART             USART1
#define COMM_BAUD              1000000U  // 1 Мбод
#define COMM_RX_DMA            DMA2  // Stream2 Ch4
#define COMM_TX_DMA            DMA2  // Stream7 Ch4
#define COMM_RX_BUF_SIZE       128U  // > max incoming frame (cascade_config = 91 B)
#define COMM_TX_BUF_SIZE       96U   // ≥ FRAME_ENCODED_SIZE(1+84) = 91
#define COMM_TX_QUEUE_LEN      4U
#endif
```

`USE_COMM_ASYNC` несумісний з `USE_HWD_UART` на одному USART: `board.c` пропускає `uart_setup()` коли `USE_COMM_ASYNC` визначено.

### CMake

```cmake
stm32_add_executable(<app> ... ${SERVOLIB_COMM}
    ${BOARD_DIR}/hwd_uart_async.c
    ${BOARD_DIR}/hwd_crc32.c)
target_include_directories(<app> PRIVATE ${CMAKE_SOURCE_DIR}/Src ${SERVOLIB_COMM_INCLUDES})
target_compile_definitions(<app> PRIVATE USE_COMM_ASYNC)
```

`SERVOLIB_COMM` та `SERVOLIB_COMM_INCLUDES` визначені у `cmake/ServoLib.cmake`.

## HWD Layer

`Inc/hwd/*.h` declare function signatures. `Board/STM32F411_OCM3/hwd_*.c` implement them with libopencm3.

Critical signatures (must match exactly):
```c
void HWD_Timer_DelayMs(uint32_t ms);   // void return — not Servo_Status_t
void HWD_Timer_DelayUs(uint32_t us);   // void return — not Servo_Status_t
uint32_t HWD_Timer_GetMillis(void);
uint32_t HWD_Timer_GetMicros(void);
```

**ADC DMA scan mode:**
```c
// HWD_ADC_Init() реєструє канал, присвоює handle->raw → слот у DMA буфері
// HWD_ADC_StartScan() конфігурує ADC1 scan+continuous + DMA2 Stream0 Ch0 circular
// HWD_ADC_ReadVoltage() читає *handle->raw — миттєво, без blocking
// Підтримка довільної кількості каналів (струм + напруга) до HWD_ADC_MAX_CHANNELS=8
// Усі канали мають використовувати один adc_base (ADC1)
```

**I2C continuous read:**
```c
HWD_I2C_StartContinuousRead(handle, dev_addr, reg, volatile_buf, size);
// Запускає I2C IT цикл: TC IRQ копіює дані в buf та перезапускає читання
```

Hardware pin assignments are in `Board/STM32F411_OCM3/board_config.h`. Driver selection macros (`USE_MOTOR_PWM`, `USE_BRAKE`, `USE_SENSOR_AS5600`, `USE_SENSOR_ACS712`, `USE_COMM_ASYNC`, etc.) are defined there or via CMake `target_compile_definitions`.

`compile_commands.json` (symlink) і `.clangd` генеруються автоматично при `cmake` configure — не редагувати вручну.

## Technical Specifications

- Control loop: 1 kHz
- PWM frequency: 20 kHz, 1000 steps resolution
- Emergency stop response: <10ms
- Memory: static allocation only — no `malloc`/`free`
- Standard: C99

## Safety-Critical Components

- `Src/ctrl/safety.c` — position/velocity/current limits, watchdog
- `Src/drv/brake/brake.c` — fail-safe brake logic
- `Servo_EmergencyStop()` — must complete within 10ms
- All error paths must leave motor stopped and brakes engaged

## Project Notes

- Code comments and docs are primarily in Ukrainian
- Main branch: `master`; feature branches merged to `master`
- Target platform: STM32F411CEU6 (BlackPill)
- If you believe there's a better solution to something, explain it in text before writing code
