# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

**ServoLib** is a modular, portable C library for controlling DC servo drives on STM32F4 platforms. The library is built on layered architecture principles with complete hardware abstraction, ensuring portability, testability, and scalability.

## Build Commands

### STM32 Hardware Build
This library integrates with STM32CubeIDE projects:

1. **Add include paths** in Project Properties → C/C++ Build → Settings:
   ```
   ../ServoLib/Inc
   ../ServoLib/Board/STM32F411
   ```

2. **Add source files** to your project:
   ```
   ServoLib/Src/**/*.c
   ServoLib/Board/STM32F411/*.c
   ```

3. Build through STM32CubeIDE (Project → Build Project)

### Testing
Testing is done on real hardware with STM32F411CEU6 (BlackPill).

## Configuration System

ServoLib uses a **three-layer configuration system** to support multiple projects while keeping the library code unchanged:

### Configuration Layers

**1. Library Constants** (`Inc/config_lib.h`)
- Universal mathematical constants (PI, unit conversions)
- Cannot be overridden
- Same for all projects

**2. User Configuration** (`YourProject/Inc/config_user.h`)
- Project-specific parameters
- **Highest priority** - overrides defaults
- Located OUTSIDE ServoLib directory

**3. Library Defaults** (`Inc/config_defaults.h`)
- Recommended defaults for typical hardware
- All parameters wrapped in `#ifndef` for override capability
- Used when not defined in config_user.h

### Configuration Order (CRITICAL!)

```
config_lib.h → config_user.h → config_defaults.h
```

This order allows config_user.h to override ANY parameter in config_defaults.h, including "absolute" limits for different hardware classes.

### How to Configure Your Project

**1. Copy the template:**
```bash
cp ServoLib/Templates/config_user_template.h YourProject/Inc/config_user.h
```

**2. Edit config_user.h with your parameters:**
```c
// For a low-power robot
#define MOTOR_MAX_CURRENT 2000U        // 2A motor
#define ENCODER_RESOLUTION_BITS 12     // AS5600 sensor
#define DEFAULT_PID_KP 2.5f

// For industrial servo (override absolute limits!)
#define ABSOLUTE_MAX_CURRENT 20000U    // 20A capable hardware
#define MOTOR_MAX_CURRENT 15000U       // 15A motor
#define ENCODER_RESOLUTION_BITS 18     // AEAT-9922 sensor
```

**3. Configure STM32CubeIDE include paths:**
```
../ServoLib/Inc
../ServoLib/Board/STM32F411
Inc  (where config_user.h is located)
```

**4. Build - the library will automatically use your configuration!**

### Configurable Parameters

**Safety Limits** (different for different hardware):
- `ABSOLUTE_MAX_CURRENT` - Maximum current capability
- `ABSOLUTE_MAX_TEMP` - Maximum temperature rating
- `MOTOR_MAX_CURRENT`, `MOTOR_OVERCURRENT_LIMIT`
- `MOTOR_MAX_TEMPERATURE`

**Control Parameters**:
- `DEFAULT_PID_KP`, `DEFAULT_PID_KI`, `DEFAULT_PID_KD`
- `DEFAULT_MAX_VELOCITY`, `DEFAULT_MAX_ACCELERATION`
- `CONTROL_LOOP_FREQ`

**Sensor Configuration**:
- `ENCODER_RESOLUTION_BITS` (12 for AS5600, 18 for AEAT-9922)
- `SENSOR_I2C_TIMEOUT`, `SENSOR_SPI_TIMEOUT`

**Feature Flags**:
- `ENABLE_TRAJECTORY_GEN`, `ENABLE_DEBUG`
- `SUPPORT_VELOCITY_MODE`, `SUPPORT_AUTO_CALIBRATION`

**See** `Templates/config_user_template.h` for complete list with examples.

### Configuration Examples

The template includes three complete examples:
1. **Low-power robot** - standard 2A motor with AS5600
2. **Industrial servo** - 15A motor with AEAT-9922, overridden absolute limits
3. **Precision positioner** - high-precision PID with S-curve trajectories

### Validation

The configuration system performs logical consistency checks at compile-time:
- Warns if `MOTOR_OVERCURRENT_LIMIT <= MOTOR_MAX_CURRENT`
- Warns if parameters exceed absolute limits
- Displays active configuration during compilation

### Benefits

- **Portability**: ServoLib/ directory unchanged between projects
- **Flexibility**: Each project configures only its parameters
- **Scalability**: Supports different hardware classes (hobby to industrial)
- **Safety**: Compile-time validation catches configuration errors

## Architecture

ServoLib uses a **5-layer architecture** for complete hardware abstraction:

```
Application (main.c)           ← Your code
    ↓
Control Layer (ctrl/)          ← PID, Safety, Trajectory, Servo
    ↓
Interface Layer (iface/)       ← Brake interface
    ↓
Driver Layer (drv/)            ← Motor, Position sensors, Brake drivers
    ↓
Hardware Driver Layer (hwd/)   ← PWM, I2C, SPI, GPIO, Timer abstractions
    ↓
Platform Layer (Board/)        ← STM32F411 implementations
```

**Key principle:** Logic layers (ctrl/, iface/, drv/) are completely independent of hardware. Only the `Board/` directory knows about the specific platform (STM32 HAL).

### Layer Responsibilities

**ctrl/** - High-level control logic:
- `servo.c` - Main servo controller coordinating all components
- `pid.c` - PID controller with anti-windup
- `safety.c` - Safety system (limits, protections, watchdog)
- `traj.c` - Trajectory generator (linear, S-curves)
- `err.c` - Error handling and logging
- `time.c` - Timing management
- `calib.c` - Calibration system

**iface/** - Abstract interfaces defining contracts:
- `brake.h` - Brake interface (release, engage, notify_activity)

**drv/** - Hardware drivers using HWD abstractions:
- `drv/motor/motor.h` - **Universal motor interface with hardware callbacks**
  - Contains `Motor_Interface_t` structure with base logic and hardware callbacks
  - Supports DC, Stepper, and BLDC motors through universal `Motor_Command_t`
  - Base functions: `Motor_Init()`, `Motor_SetPower()`, `Motor_Stop()`, `Motor_EmergencyStop()`
  - Wrapper functions: `Motor_SetPower_DC()`, `Motor_SetPower_Stepper()`, `Motor_SetPower_BLDC()`
- `drv/motor/pwm.c` - PWM motor driver implementation
  - Provides hardware callbacks for `Motor_Interface_t`
  - Supports single-channel (PWM+DIR) and dual-channel (H-bridge) modes
  - Create with `PWM_Motor_Create()`, then use `&driver->interface` for motor operations
- `drv/position/position.h` - **Position sensor interface** (read_angle, get_velocity, calibrate)
  - Universal interface for all position sensors (magnetic encoders, absolute encoders, etc.)
  - Contains `Sensor_Interface_t` with callback functions pattern
- `drv/position/aeat9922.c` - AEAT-9922 18-bit magnetic encoder (SPI) - **Currently enabled**
- `drv/position/as5600.c` - AS5600 12-bit magnetic encoder (I2C) - Currently disabled
- `drv/brake/brake.c` - Electronic brake driver with fail-safe logic

**hwd/** - Hardware abstraction layer (declarations only):
- `hwd_pwm.h` - PWM abstraction
- `hwd_i2c.h` - I2C abstraction
- `hwd_spi.h` - SPI abstraction
- `hwd_gpio.h` - GPIO abstraction
- `hwd_timer.h` - Timer abstraction

**Board/** - Platform-specific implementations:
- `Board/STM32F411/` - STM32F411 HAL implementations

### HWD Implementation Patterns

**Critical:** HWD implementations in `Board/` must match the function signatures defined in `Inc/hwd/` headers exactly.

**Common HWD Functions:**
```c
// Inc/hwd/hwd_timer.h declares:
uint32_t HWD_Timer_GetMillis(void);
uint32_t HWD_Timer_GetMicros(void);
void HWD_Timer_DelayMs(uint32_t ms);  // Note: void return
void HWD_Timer_DelayUs(uint32_t us);  // Note: void return

// Inc/hwd/hwd_pwm.h declares:
Servo_Status_t HWD_PWM_SetDuty(HWD_PWM_Handle_t* handle, uint32_t duty);
Servo_Status_t HWD_PWM_SetDutyPercent(HWD_PWM_Handle_t* handle, float percent);

// Inc/hwd/hwd_gpio.h declares:
Servo_Status_t HWD_GPIO_WritePin(void* port, uint16_t pin, HWD_GPIO_PinState_t state);
Servo_Status_t HWD_GPIO_WritePinDescriptor(const HWD_GPIO_Pin_t* pin, HWD_GPIO_PinState_t state);
```

## Development Guidelines

### C Standard and Memory
- **C99 standard** - no C++ features
- **Static memory allocation only** - no malloc/free
- All structures pre-allocated at compile time

### Hardware Abstraction Rules
1. **Never call STM32 HAL directly** from ctrl/, iface/, or drv/ layers
2. **Always use HWD functions** for hardware access
3. **Platform code only in Board/** - all other code must be portable
4. When adding new hardware support, only modify/create files in `Board/YOUR_PLATFORM/`

### Motor Driver Development Rules (After Refactoring)
1. **Never modify Motor_Interface_t directly** - use only through provided functions
2. **Base logic in motor.c** - Common operations (state, power limiting, statistics) handled by `Motor_Init()`, `Motor_SetPower()`, `Motor_Stop()`
3. **Hardware specifics in callbacks** - Each driver (PWM, Stepper, BLDC) provides hardware callbacks
4. **Use wrapper functions** - Prefer `Motor_SetPower_DC()` over creating `Motor_Command_t` manually
5. **Driver creation pattern:**
   - Call `SpecificDriver_Create()` to set up hardware callbacks
   - Access interface via `&driver->interface`
   - Initialize with `Motor_Init(&driver->interface, params)`

### Adding New Motor Type Support
To add a new motor type (e.g., BLDC, Stepper):

1. **Create driver file** `drv/motor/your_motor.h` and `drv/motor/your_motor.c`
2. **Define driver structure:**
   ```c
   typedef struct {
       Motor_Interface_t interface;  // Universal interface
       // Your hardware-specific fields
   } YourMotor_Driver_t;
   ```
3. **Implement hardware callbacks:**
   - `YourMotor_HW_Init()` - Initialize hardware (timers, GPIO, etc.)
   - `YourMotor_HW_SetPower()` - Apply power to motor phases
   - `YourMotor_HW_Stop()` - Stop motor
   - `YourMotor_HW_Update()` - Update state (optional)
4. **Create factory function:**
   ```c
   Servo_Status_t YourMotor_Create(YourMotor_Driver_t* driver, config) {
       driver->interface.hw.init = YourMotor_HW_Init;
       driver->interface.hw.set_power = YourMotor_HW_SetPower;
       // ... other callbacks
       driver->interface.driver_data = driver;
   }
   ```
5. **Use universal interface** - All base logic (state, stats) is handled by `motor.c`

### Adding New Platform Support
To port to another STM32 or platform:

1. Create `Board/YOUR_PLATFORM/` directory
2. Copy from existing platform: `cp -r Board/STM32F411/* Board/YOUR_PLATFORM/`
3. Update `board_config.h` with your pins, timers, and peripherals
4. Implement HWD functions for your platform (hwd_pwm.c, hwd_i2c.c, etc.)
5. No changes needed in ctrl/, iface/, or drv/ layers

### Conditional Compilation System

**Driver Selection Macros** (`Board/*/board_config.h`):
- `USE_MOTOR_PWM` - Enable PWM motor driver
- `USE_BRAKE` - Enable brake driver
- `USE_SENSOR_AS5600` - Enable AS5600 I2C encoder
- `USE_SENSOR_AEAT9922` - Enable AEAT-9922 SPI encoder

**Pattern:**
```c
// In board_config.h for STM32F411:
#define USE_REAL_HARDWARE   1
#define USE_MOTOR_PWM       1
#define USE_BRAKE           1
```

**Driver Implementation:**
Each driver file uses conditional compilation:
```c
#include "board_config.h"
#ifdef USE_MOTOR_PWM
// Hardware implementation
#endif
```

### File Organization
- Headers in `Inc/` mirror implementations in `Src/`
- Each driver has separate files for hardware (drv/) vs interface (iface/)
- All driver .c files use conditional compilation based on board_config.h macros

### Error Handling
- All functions return `Servo_Status_t` enum
- Status codes: SERVO_OK, SERVO_ERROR, SERVO_BUSY, SERVO_TIMEOUT, SERVO_INVALID, SERVO_NOT_INIT, SERVO_ERROR_NULL_PTR
- Safety-critical operations (brake, emergency stop) must complete within 10ms

### Naming Conventions
- **Modules:** PascalCase (e.g., `Motor_Interface_t`, `Servo_Controller_t`)
- **Functions:** Module_Action (e.g., `Servo_Init()`, `Motor_SetPower()`)
- **Constants:** UPPER_SNAKE_CASE (e.g., `MOTOR_MAX_CURRENT`)
- **Private functions:** Static with internal_ prefix
- **HWD functions:** HWD_Module_Action (e.g., `HWD_PWM_SetDuty()`)

## Key Technical Specifications

### Control System
- Control loop frequency: 1 kHz (1ms period)
- Position control with PID regulation
- Trajectory generation (linear and S-curve profiles)
- Safety limits: position, velocity, current
- Emergency stop response time: <10ms

### Motor Control
- PWM frequency: Configurable, default 1 kHz
- PWM resolution: 1000 steps (0.1%)
- Dual-channel mode for H-bridge drivers (L298N, TB6612, etc.)
- Direction inversion support
- Overcurrent protection

### Sensors
- **AEAT-9922: 18-bit SPI magnetic encoder (262144 counts/rev)** - Currently enabled and in use
- AS5600: 12-bit I2C magnetic encoder (4096 counts/rev) - Available but currently disabled (I2C not configured)
- Velocity calculation from position delta
- Anomaly filtering

### Brakes
- Fail-safe electronic brakes (engaged when unpowered)
- Automatic release before motion
- Inactivity timeout with auto-engage
- Emergency engage on critical errors

### STM32F411 Hardware Requirements
- **Timer 3:** PWM for motor (CH1: PA6, CH2: PA7)
- **I2C1:** Sensor AS5600 (SCL: PB6, SDA: PB7)
- **SPI1:** Sensor AEAT-9922 (SCK: PA5, MISO: PA6, MOSI: PA7, CS: PA4)
- **Timer 5:** 32-bit microsecond timer
- **GPIO:** Brake control (e.g., PA8)

## Motor Driver Architecture (After Refactoring)

### Key Concepts

**Separation of Concerns:**
- **Motor_Interface_t** (`drv/motor/motor.h`) - Universal interface containing:
  - `Motor_Data_t data` - Common logic: state, power, direction, statistics
  - `Motor_Hardware_Callbacks_t hw` - Hardware-specific callbacks
  - `void* driver_data` - Pointer to specific driver (e.g., `PWM_Motor_Driver_t`)

- **Specific Driver** (e.g., `PWM_Motor_Driver_t` in `drv/motor/pwm.h`) - Contains:
  - `Motor_Interface_t interface` - The universal interface
  - Hardware-specific configuration (PWM channels, GPIO pins)
  - Provides hardware callbacks to the interface

**Universal Command System:**
```c
Motor_Command_t cmd = {
    .type = MOTOR_TYPE_DC_PWM,  // or MOTOR_TYPE_STEPPER, MOTOR_TYPE_BLDC
    .data.dc.power = 50.0f      // DC motor: -100.0 to +100.0
    // .data.stepper = {phase_a, phase_b}      // Stepper motor
    // .data.bldc = {phase_a, phase_b, phase_c} // BLDC motor
};
Motor_SetPower(&motor_interface, &cmd);
```

**Hardware Callbacks Pattern:**
```c
// PWM driver provides these callbacks
Motor_Hardware_Callbacks_t hw = {
    .init = PWM_HW_Init,          // Initialize PWM channels
    .deinit = PWM_HW_DeInit,      // Deinitialize
    .set_power = PWM_HW_SetPower, // Apply PWM signals
    .stop = PWM_HW_Stop,          // Stop PWM
    .update = PWM_HW_Update       // Update (optional)
};
```

### Common Patterns

#### 1. Creating and Initializing PWM Motor Driver
```c
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"

PWM_Motor_Driver_t motor_driver;
Servo_Controller_t servo;

// Step 1: Create PWM driver (sets up hardware callbacks)
PWM_Motor_Config_t pwm_config = {
    .type = PWM_MOTOR_TYPE_DUAL_PWM,  // H-bridge mode
    .pwm_fwd = &pwm_handle_fwd,       // Forward PWM channel
    .pwm_bwd = &pwm_handle_bwd,       // Backward PWM channel
    .gpio_dir = NULL                  // No DIR pin in dual-PWM mode
};
PWM_Motor_Create(&motor_driver, &pwm_config);

// Step 2: Initialize motor interface (calls hw.init callback)
Motor_Params_t motor_params = {
    .type = MOTOR_TYPE_DC_PWM,
    .max_power = 100.0f,
    .min_power = 5.0f,
    .invert_direction = false
};
Motor_Init(&motor_driver.interface, &motor_params);

// Step 3: Configure servo with motor interface
Servo_Config_t servo_config = {
    .update_frequency = 1000.0f,
    /* ... PID, safety, trajectory params ... */
};
Servo_Init(&servo, &servo_config, &motor_driver.interface);

// Main control loop
while(1) {
    Servo_Update(&servo);  // Call at 1kHz
    HAL_Delay(1);
}
```

#### 2. Direct Motor Control (Without Servo)
```c
// Using universal Motor_SetPower() with command structure
Motor_Command_t cmd = {
    .type = MOTOR_TYPE_DC_PWM,
    .data.dc.power = 75.0f  // 75% forward
};
Motor_SetPower(&motor_driver.interface, &cmd);

// Using wrapper function for convenience
Motor_SetPower_DC(&motor_driver.interface, -50.0f);  // 50% backward

// Stop motor
Motor_Stop(&motor_driver.interface);

// Emergency stop
Motor_EmergencyStop(&motor_driver.interface);
```

#### 3. Setting Target Position (With Servo)
```c
// Set position in degrees
Servo_SetPosition(&servo, 90.0f);

// Check if target reached
if (Servo_IsAtTarget(&servo)) {
    // Target reached
}

// Read current state
float position = Servo_GetPosition(&servo);
float velocity = Servo_GetVelocity(&servo);
Servo_State_t state = Servo_GetState(&servo);
```

#### 4. Single-Channel PWM Mode (PWM + DIR)
```c
PWM_Motor_Config_t pwm_config = {
    .type = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,  // PWM + DIR mode
    .pwm_fwd = &pwm_handle,                 // Single PWM channel
    .pwm_bwd = NULL,                        // Not used
    .gpio_dir = GPIOA,                      // Direction GPIO port
    .gpio_pin = GPIO_PIN_8                  // Direction pin
};
PWM_Motor_Create(&motor_driver, &pwm_config);
```

## Documentation Files

- `Doc/README.md` - Quick start guide (in Ukrainian)
- `Doc/structure.md` - Detailed project structure
- `Doc/technical_specifications.md` - Complete technical specifications
- `Doc/BRAKE_DRIVER.md` - Brake system documentation
- `Doc/AEAT-9922_DRIVER_GUIDE.md` - AEAT-9922 encoder guide
- `Doc/ARCHITECTURE_PLAN.md` - Motor driver architecture plan
- `Templates/config_user_template.h` - **Configuration template with examples**
- `Examples/` - Usage examples for motors and sensors

## Git Workflow

Main branch: `master`
Development happens on feature branches merged to `master`

## Important Notes

- **Language:** Code comments and documentation are primarily in Ukrainian
- **License:** MIT License
- **Organization:** КПІ ім. Ігоря Сікорського (diploma project)
- **Target platform:** STM32F411CEU6 (BlackPill) as primary, but architecture supports any STM32F4
- **Current sensor:** AEAT-9922 (18-bit SPI) is enabled; AS5600 (I2C) is available but disabled
- **Motor types:** DC motors with PWM control (supports both single-channel PWM+DIR and dual-channel H-bridge modes)
- **Motor architecture:** Unified `Motor_Interface_t` with hardware callbacks pattern, extensible to Stepper and BLDC motors

## Safety Critical Code

The following components are safety-critical and require careful handling:
- `ctrl/safety.c` - Implements position/velocity/current limits and emergency stop
- `drv/brake/brake.c` - Fail-safe brake logic (brakes engage on power loss)
- `Servo_EmergencyStop()` - Must execute within 10ms
- All error handling paths must ensure safe state (motor stopped, brakes engaged)
