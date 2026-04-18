# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

**ServoLib** is a modular, portable C library for controlling DC servo drives on STM32F4 platforms using **libopencm3** (not STM32 HAL). Built on a 5-layer architecture with complete hardware abstraction.

## Build Commands

**Prerequisites:** Set `LIBOPENCM3_DIR` before configuring:
```bash
export LIBOPENCM3_DIR=/path/to/libopencm3
```

**Build targets** (`Apps/`): `debug_encoder`, `debug_motor`, `debug_brake`, `servo_full`

```bash
# Configure (select target interactively or pass name):
./configure.sh servo_full        # or: ./configure.sh debug_encoder, etc.

# Build:
./build.sh                       # builds whichever target was last configured

# Flash to board:
./flash.sh stlink                # or: ./flash.sh daplink
```

Under the hood, `configure.sh` runs `cmake --preset <target>` and symlinks `compile_commands.json` to the project root for LSP/clangd. `build.sh` detects the current target from that symlink.

**No unit test framework exists.** Testing is done on real hardware (STM32F411CEU6 BlackPill) via UART output from the debug apps.

## Architecture

```
Apps/                          ← Debug/application targets (main.c per target)
    ↓
Src/ctrl/                      ← PID, Safety, Trajectory, Servo coordinator
    ↓
Src/drv/                       ← Motor, Position sensor, Brake drivers
    ↓
Inc/hwd/ + Board/STM32F411_OCM3/  ← HWD declarations + libopencm3 implementations
```

**Key principle:** `ctrl/`, `drv/` are fully hardware-independent. Only `Board/` imports libopencm3. Never call libopencm3 functions from `ctrl/` or `drv/`.

### Universal Interface Pattern

All three driver types (Motor, Position, Brake) share the same pattern:

1. **Universal interface struct** (`Motor_Interface_t`, `Position_Sensor_Interface_t`, `Brake_Interface_t`) — contains common data + hardware callback function pointers
2. **Specific driver struct** (e.g., `PWM_Motor_Driver_t`) — has the interface as its **first field**, plus hardware-specific state
3. **Factory function** (e.g., `PWM_Motor_Create()`) — fills in the hardware callbacks
4. **Base functions** in `motor.c`/`position.c`/`brake.c` — implement common logic, call callbacks for hardware ops

Access the universal interface via `&driver->interface`. The specific driver struct can be safely cast to/from `Motor_Interface_t*` because interface is the first field.

### Servo Initialization

```c
// Full initialization with all optional components:
Servo_InitFull(&servo, &config, &motor->interface, &sensor->interface, &brake->interface);

// Motor only (sensor and brake = NULL):
Servo_Init(&servo, &config, &motor->interface);
```

`Servo_InitWithBrake()` does **not exist** — use `Servo_InitFull()` with `brake` parameter.

## Configuration System

Three-layer system resolved at compile time via `Inc/config.h`:

1. `Inc/config_lib.h` — math constants (PI, conversions), cannot be overridden
2. `Inc/config_user.h` — project-specific values, **highest priority**
3. `Inc/config_defaults.h` — library defaults (all wrapped in `#ifndef`)

**Include order:** `config_lib.h` → `config_user.h` → `config_defaults.h`

In this repository, `config_user.h` lives inside `Inc/`. The template at `Templates/config_user_template.h` shows three complete example configurations.

## Drivers

### Position Sensors

| Sensor | Resolution | Interface | Status |
|--------|-----------|-----------|--------|
| AEAT-9922 | 18-bit (262144 cpr) | SPI | **Active** |
| AS5600 | 12-bit (4096 cpr) | I2C | Available, disabled |
| Incremental | Quadrature via TIM2 | GPIO | Available |

**Critical:** `HW_ReadRaw()` callbacks must return **only raw counts** — never degrees. All conversion (raw→degrees, velocity, multi-turn, prediction) is done in `position.c`.

### Motor Drivers

PWM motor supports two modes:
- `PWM_MOTOR_TYPE_SINGLE_PWM_DIR` — one PWM channel + DIR GPIO
- `PWM_MOTOR_TYPE_DUAL_PWM` — two PWM channels (H-bridge like L298N, TB6612)

### Brake Driver

State machine: `ENGAGED ↔ ENGAGING ↔ RELEASING ↔ RELEASED`

Brake always initializes to `ENGAGED` (fail-safe). Call `Brake_Update()` in the control loop to process timed transitions.

## HWD Layer

`Inc/hwd/*.h` declare function signatures. `Board/STM32F411_OCM3/hwd_*.c` implement them with libopencm3.

Critical signatures (must match exactly):
```c
void HWD_Timer_DelayMs(uint32_t ms);   // void return — not Servo_Status_t
void HWD_Timer_DelayUs(uint32_t us);   // void return — not Servo_Status_t
uint32_t HWD_Timer_GetMillis(void);
uint32_t HWD_Timer_GetMicros(void);
```

Hardware pin assignments are in `Board/STM32F411_OCM3/board_config.h`. Driver selection macros (`USE_MOTOR_PWM`, `USE_BRAKE`, `USE_SENSOR_AEAT9922`, etc.) are also defined there.

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
