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

**Build targets** (`Apps/`): `debug_encoder`, `debug_motor`, `debug_brake`, `servo_full`

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
    ↓
Src/drv/                       ← Motor, Position sensor, Brake, Current drivers
    ↓
Inc/hwd/ + Board/STM32F411_OCM3/  ← HWD declarations + libopencm3 implementations
```

**Key principle:** `ctrl/`, `drv/` are fully hardware-independent. Only `Board/` imports libopencm3. Never call libopencm3 functions from `ctrl/` or `drv/`.

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
ACS712_Create(&driver, &config);   // factory
Current_Sensor_Calibrate(&driver.interface);  // при нульовому струмі

// У control loop (оновлення EMA фільтра):
Current_Sensor_Update(&driver.interface);
// Servo_Update() сам читає струм через GetCurrent — не потрібно викликати у servo loop
Current_Sensor_GetCurrent(&driver.interface, &current_a);
```

`Current_Sensor_GetStats`, `Current_Sensor_DeInit` — **не існують**.

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
ff = ff_j * current_sp + ff_b * omega   // в %
// ff_j [%/А], ff_b [%·с/рад]
// У CASCADE_MODE_TRQ ff_b не застосовується
```

**Slew rate:** `config.slew_rate` [%/с] обмежує зміну команди двигуну. `0` = вимкнено.

**`Cascade_PID_Params_t`** для кожного контуру: `kp`, `ki`, `kd`, `out_min`, `out_max`, `i_limit`.
`i_limit > 0` — клемпує внесок інтегратора до `±i_limit` (в одиницях виходу).

**`PID_Params_t.i_limit`**: якщо `> 0` — незалежний ліміт інтегратора; `= 0` — старий механізм (clamp відносно `out_min/out_max - p_term`).

**`pid_mgr.c` видалено з `SERVOLIB_CTRL`** — замінено `cascade.c`.

## Safety

`Safety_Config_t` (всі одиниці в радіанах/рад·с):
- `min_position`, `max_position` — рад
- `max_velocity` — рад/с
- `max_acceleration` — рад/с²
- `critical_current_a` — А (аварійне вимкнення при перевищенні + таймаут)

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

Hardware pin assignments are in `Board/STM32F411_OCM3/board_config.h`. Driver selection macros (`USE_MOTOR_PWM`, `USE_BRAKE`, `USE_SENSOR_AS5600`, `USE_SENSOR_ACS712`, etc.) are defined there.

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
