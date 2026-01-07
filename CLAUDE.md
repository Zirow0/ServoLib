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
- `pid_mgr.c` - PID manager for cascade control

**iface/** - Abstract interfaces defining contracts:
- Більше не використовується (компоненти перенесено в drv/)

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
- `drv/position/position.h` - **Universal position sensor interface with hardware callbacks**
  - Contains `Position_Sensor_Interface_t` with base logic (velocity, multi-turn, prediction) and hardware callbacks
  - Universal interface for all position sensors (magnetic encoders, absolute encoders, SSI, etc.)
  - Base functions: `Position_Sensor_Init()`, `Position_Sensor_Update()`, `Position_Sensor_GetPosition()`, `Position_Sensor_GetVelocity()`
  - Advanced features: multi-turn tracking, prediction (extrapolation), zero calibration
- `drv/position/aeat9922.c` - AEAT-9922 18-bit magnetic encoder (SPI) - **Currently enabled**
  - Provides hardware callbacks for `Position_Sensor_Interface_t`
  - Create with `AEAT9922_Create()`, then use `&driver->interface` for sensor operations
- `drv/position/as5600.c` - AS5600 12-bit magnetic encoder (I2C) - Currently disabled
- `drv/brake/brake.h` - **Universal brake interface with hardware callbacks**
  - Contains `Brake_Interface_t` structure with base logic and hardware callbacks
  - Supports different brake types (electromagnetic, pneumatic, hydraulic) through universal interface
  - Base functions: `Brake_Init()`, `Brake_Engage()`, `Brake_Release()`, `Brake_EmergencyEngage()`, `Brake_Update()`
  - State machine with transitions: ENGAGED, RELEASED, ENGAGING, RELEASING
- `drv/brake/gpio_brake.c` - GPIO brake driver implementation
  - Provides hardware callbacks for `Brake_Interface_t`
  - Controls electromagnetic brakes via GPIO (active high/low configurable)
  - Create with `GPIO_Brake_Create()`, then use `&driver->interface` for brake operations

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

## Brake Driver Architecture (After Refactoring)

### Key Concepts

**Separation of Concerns:**
- **Brake_Interface_t** (`drv/brake/brake.h`) - Universal interface containing:
  - `Brake_Data_t data` - Common logic: state, transition timing, initialization flag
  - `Brake_Hardware_Callbacks_t hw` - Hardware-specific callbacks
  - `void* driver_data` - Pointer to specific driver (e.g., `GPIO_Brake_Driver_t`)

- **Specific Driver** (e.g., `GPIO_Brake_Driver_t` in `drv/brake/gpio_brake.h`) - Contains:
  - `Brake_Interface_t interface` - The universal interface (FIRST field!)
  - Hardware-specific configuration (GPIO port, pin, active_high)
  - Provides hardware callbacks to the interface

**State Machine with Transitions:**
```
ENGAGED ←→ ENGAGING (transition: release_time_ms)
RELEASED ←→ RELEASING (transition: engage_time_ms)
```

**Hardware Callbacks Pattern:**
```c
// GPIO driver provides these callbacks
Brake_Hardware_Callbacks_t hw = {
    .init = GPIO_Brake_HW_Init,          // Initialize GPIO
    .deinit = GPIO_Brake_HW_Deinit,      // Deinitialize
    .engage = GPIO_Brake_HW_Engage,      // Set GPIO to active state
    .release = GPIO_Brake_HW_Release     // Set GPIO to inactive state
};
```

### Common Patterns

#### 1. Creating and Initializing GPIO Brake Driver
```c
#include "ctrl/servo.h"
#include "drv/brake/gpio_brake.h"

GPIO_Brake_Driver_t brake_driver;
Servo_Controller_t servo;

// Step 1: Create GPIO brake driver (sets up hardware callbacks)
GPIO_Brake_Config_t brake_config = {
    .gpio_port = GPIOA,
    .gpio_pin = GPIO_PIN_8,
    .active_high = false,           // Active LOW (brake engages when GPIO=LOW)
    .engage_time_ms = 50,           // 50ms to fully engage
    .release_time_ms = 30           // 30ms to fully release
};
GPIO_Brake_Create(&brake_driver, &brake_config);

// Step 2: Configure servo with brake interface
Servo_Config_t servo_config = {
    .update_frequency = 1000.0f,
    .enable_brake = true,
    /* ... PID, safety, trajectory params ... */
};
Servo_InitWithBrake(&servo, &servo_config, &motor_driver.interface, &brake_driver.interface);

// Main control loop
while(1) {
    Servo_Update(&servo);  // Automatically calls Brake_Update()
    HAL_Delay(1);
}
```

#### 2. Direct Brake Control (Without Servo)
```c
// Engage brakes (blocking movement)
Brake_Engage(&brake_driver.interface);

// Check state during transition
Brake_State_t state = Brake_GetState(&brake_driver.interface);
if (state == BRAKE_STATE_ENGAGING) {
    // Brakes are transitioning, wait...
}

// Update in loop to handle transitions
while (!Brake_IsEngaged(&brake_driver.interface)) {
    Brake_Update(&brake_driver.interface);
    HAL_Delay(1);
}

// Release brakes (allow movement)
Brake_Release(&brake_driver.interface);

// Emergency engage (same as Engage for GPIO, but different for pneumatic/hydraulic)
Brake_EmergencyEngage(&brake_driver.interface);
```

#### 3. Checking Brake State
```c
// Get current state
Brake_State_t state = Brake_GetState(&brake_driver.interface);

// Check if in stable state
if (Brake_IsEngaged(&brake_driver.interface)) {
    // Brakes are fully engaged (stable)
}

if (Brake_IsReleased(&brake_driver.interface)) {
    // Brakes are fully released (stable)
}

// Check if transitioning
if (Brake_IsTransitioning(&brake_driver.interface)) {
    // Brakes are ENGAGING or RELEASING
}
```

### Brake Driver Development Rules

1. **Never modify Brake_Interface_t directly** - use only through provided functions
2. **Base logic in brake.c** - State transitions, timing handled by `Brake_Init()`, `Brake_Update()`
3. **Hardware specifics in callbacks** - Each driver (GPIO, Pneumatic, Hydraulic) provides hardware callbacks
4. **Driver creation pattern:**
   - Call `GPIO_Brake_Create()` to set up hardware callbacks
   - Access interface via `&driver->interface`
   - Initialize with `Brake_Init()` (called internally by Create)
   - Update regularly with `Brake_Update()` to process transitions

### Adding New Brake Type Support

To add a new brake type (e.g., Pneumatic, Hydraulic):

1. **Create driver file** `drv/brake/your_brake.h` and `drv/brake/your_brake.c`
2. **Define driver structure:**
   ```c
   typedef struct {
       Brake_Interface_t interface;  // Universal interface (FIRST!)
       // Your hardware-specific fields (valve control, pressure sensor, etc.)
   } YourBrake_Driver_t;
   ```
3. **Implement hardware callbacks:**
   - `YourBrake_HW_Init()` - Initialize hardware (valves, sensors)
   - `YourBrake_HW_Engage()` - Physically engage brakes
   - `YourBrake_HW_Release()` - Physically release brakes
   - `YourBrake_HW_Deinit()` - Cleanup (optional)
4. **Create factory function:**
   ```c
   Servo_Status_t YourBrake_Create(YourBrake_Driver_t* driver, config) {
       driver->interface.hw.init = YourBrake_HW_Init;
       driver->interface.hw.engage = YourBrake_HW_Engage;
       driver->interface.hw.release = YourBrake_HW_Release;
       driver->interface.driver_data = driver;

       Brake_Params_t params = {
           .engage_time_ms = config->engage_time_ms,
           .release_time_ms = config->release_time_ms
       };
       return Brake_Init(&driver->interface, &params);
   }
   ```
5. **Use universal interface** - All base logic (state machine, transitions) handled by `brake.c`

### Key Differences from Motor/Position Drivers

- **No automatic mode** - User controls engage/release explicitly (no idle timeout)
- **Mandatory transitions** - ENGAGING/RELEASING states ensure proper timing for electromagnetic actuation
- **Fail-safe by default** - Always initializes to ENGAGED state
- **Emergency engage** - Separate function for critical situations (same as Engage for GPIO)

## Position Sensor Architecture (After Refactoring)

### Key Concepts

**Separation of Concerns:**
- **Position_Sensor_Interface_t** (`drv/position/position.h`) - Universal interface containing:
  - `Position_Sensor_Data_t data` - Common logic: position, velocity, multi-turn tracking, prediction
  - `Position_Sensor_HW_Callbacks_t hw` - Hardware-specific callbacks (SPI/I2C read)
  - `void* driver_data` - Pointer to specific driver (e.g., `AEAT9922_Driver_t`)

- **Specific Driver** (e.g., `AEAT9922_Driver_t` in `drv/position/aeat9922.h`) - Contains:
  - `Position_Sensor_Interface_t interface` - The universal interface (FIRST field!)
  - Hardware-specific configuration (SPI config, status registers)
  - Provides hardware callbacks to the interface

**Hardware Callbacks Pattern:**
```c
// AEAT9922 driver provides these callbacks
Position_Sensor_HW_Callbacks_t hw = {
    .init = AEAT9922_HW_Init,          // Initialize SPI
    .deinit = AEAT9922_HW_DeInit,      // Deinitialize
    .read_raw = AEAT9922_HW_ReadRaw,   // Read raw position (NO conversion!)
    .calibrate = AEAT9922_HW_Calibrate // Zero calibration
};
```

**Key Principle:**
- **Drivers read ONLY raw data** (e.g., 0-262143 for 18-bit encoder)
- **position.c does ALL processing:** raw→degrees, velocity calculation, multi-turn tracking, prediction

### Util Modules

**util/derivative.c** - Velocity calculation:
```c
float velocity = Derivative_CalculateVelocity(
    current_angle, last_angle,
    current_time_us, last_time_us
);
```

**util/prediction.c** - Position extrapolation between updates:
```c
float predicted = Prediction_GetCurrentPosition(
    last_position, velocity,
    last_time_us, current_time_us
);
```

### Common Patterns

#### 1. Creating and Initializing AEAT9922 Sensor
```c
#include "drv/position/position.h"
#include "drv/position/aeat9922.h"

AEAT9922_Driver_t encoder_driver;

// Step 1: Create AEAT9922 driver (sets up hardware callbacks)
AEAT9922_Config_t encoder_config = {
    .spi_config = { /* SPI settings */ },
    .msel_port = GPIOA,
    .msel_pin = GPIO_PIN_4,
    .abs_resolution = AEAT9922_ABS_RES_18BIT,  // 262144 counts/rev
    .incremental_cpr = 1024,
    .interface_mode = AEAT9922_INTERFACE_SPI4_24BIT,
    .direction_ccw = false,
    .enable_incremental = false
};
AEAT9922_Create(&encoder_driver, &encoder_config);

// Step 2: Initialize position sensor interface (calls hw.init callback)
Position_Params_t sensor_params = {
    .type = SENSOR_TYPE_AEAT9922,
    .resolution_bits = 18,
    .min_angle = 0.0f,
    .max_angle = 360.0f,
    .update_rate = 1000  // 1 kHz
};
Position_Sensor_Init(&encoder_driver.interface, &sensor_params);

// Step 3: Use in control loop
while(1) {
    // Update sensor (reads raw SPI data, calculates position/velocity/prediction)
    Position_Sensor_Update(&encoder_driver.interface);

    // Get processed data
    float position, velocity;
    Position_Sensor_GetPosition(&encoder_driver.interface, &position);
    Position_Sensor_GetVelocity(&encoder_driver.interface, &velocity);

    HAL_Delay(1);  // 1 kHz update rate
}
```

#### 2. Multi-turn Tracking
```c
// Enable multi-turn capability in Create()
encoder_driver.interface.capabilities = POSITION_CAP_ABSOLUTE | POSITION_CAP_MULTITURN;

// Get absolute position (can be > 360°)
float abs_position;
Position_Sensor_GetAbsolutePosition(&encoder_driver.interface, &abs_position);
// Example: 725.5° means 2 full revolutions + 5.5°
```

#### 3. Prediction (Extrapolation Between Updates)
```c
// Prediction automatically updated in Position_Sensor_Update()
// Get current predicted position (extrapolated from last measurement)
float predicted_position;
Position_Sensor_GetPredictedPosition(&encoder_driver.interface, &predicted_position);

// Useful for high-speed control loops where sensor update rate < control loop rate
```

#### 4. Zero Calibration
```c
// Calibrate current position as zero
Position_Sensor_Calibrate(&encoder_driver.interface);
// Calls AEAT9922_HW_Calibrate() → AEAT9922_CalibrateZero()

// Or set arbitrary zero position
Position_Sensor_SetPosition(&encoder_driver.interface, 0.0f);
```

### Position Sensor Development Rules

1. **Never modify Position_Sensor_Interface_t directly** - use only through provided functions
2. **Base logic in position.c** - Conversion, velocity, multi-turn, prediction handled universally
3. **Hardware specifics in callbacks** - Each driver (AEAT9922, AS5600) provides ONLY hardware operations
4. **Driver creation pattern:**
   - Call `AEAT9922_Create()` to set up hardware callbacks
   - Access interface via `&driver->interface`
   - Initialize with `Position_Sensor_Init(&driver->interface, params)`
   - Update regularly with `Position_Sensor_Update(&driver->interface)`

### Adding New Position Sensor Support

To add a new position sensor (e.g., AS5600, SSI encoder):

1. **Create driver file** `drv/position/your_sensor.h` and `drv/position/your_sensor.c`
2. **Define driver structure:**
   ```c
   typedef struct {
       Position_Sensor_Interface_t interface;  // Universal interface (FIRST!)
       // Your hardware-specific fields (I2C/SPI handle, status, etc.)
   } YourSensor_Driver_t;
   ```
3. **Implement hardware callbacks:**
   - `YourSensor_HW_Init()` - Initialize communication (SPI/I2C/SSI)
   - `YourSensor_HW_ReadRaw()` - Read ONLY raw position value, NO conversion!
   - `YourSensor_HW_Calibrate()` - Hardware-specific calibration (optional)
4. **Create factory function:**
   ```c
   Servo_Status_t YourSensor_Create(YourSensor_Driver_t* driver, config) {
       driver->interface.hw.init = YourSensor_HW_Init;
       driver->interface.hw.read_raw = YourSensor_HW_ReadRaw;
       driver->interface.capabilities = POSITION_CAP_ABSOLUTE;
       driver->interface.resolution_bits = 12;  // e.g., AS5600
       driver->interface.driver_data = driver;
   }
   ```
5. **Use universal interface** - All processing (velocity, multi-turn, prediction) handled by `position.c`

### Data Flow

```
Application
    ↓
Position_Sensor_Update(&sensor->interface)
    ↓
position.c calls sensor->hw.read_raw()
    ↓
aeat9922.c reads SPI → returns Position_Raw_Data_t { raw_position, timestamp_us, valid }
    ↓
position.c processes:
    - raw → degrees (RawToAngleDegrees)
    - velocity (Derivative_CalculateVelocity)
    - multi-turn (UpdateMultiTurn)
    - prediction (Prediction_GetCurrentPosition)
    ↓
Application gets processed data via:
    - Position_Sensor_GetPosition()
    - Position_Sensor_GetVelocity()
    - Position_Sensor_GetAbsolutePosition()
    - Position_Sensor_GetPredictedPosition()
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
- If you are sure there is a better solution, please tell me.
- When discussing, don't write code, it's better to describe it in text.
- When discussing, write concisely, but at the same time do not forget to mention important details.