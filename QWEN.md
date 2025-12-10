# ServoLib - Servo Control Library

## Overview

ServoLib is a modular, portable C library for controlling DC servo drives on the STM32F4 platform. The library is built on **layered architecture principles** with complete hardware abstraction, ensuring **portability**, **testability**, and **scalability**. It's designed for embedded systems requiring precise motor control with safety features.

## Key Features

- 🏗️ **Layered architecture** with Hardware Driver Layer (HWD)
- 🔌 **Complete hardware abstraction** - platform independent logic
- 🎛️ **PID controllers** with anti-windup
- 📊 **Trajectory generator** (linear, S-curves)
- 🛡️ **Safety system** with protections and limits
- 🔒 **Fail-safe electronic brakes**
- 📡 **Magnetic encoder support** (AS5600, AEAT-9922)
- 🧪 **Easy testing** (HWD can be mocked)
- 🚀 **Ready for real-world projects**
- 🔧 **PC Emulation via UDP** - для відладки на персональному комп'ютері

## Architecture

ServoLib uses a **5-layer architecture**:

```
┌───────────────────────────────────────┐
│     Application (main.c)              │  ← Your code
├───────────────────────────────────────┤
│     Control Layer (ctrl/)             │  ← PID, Safety, Trajectory
├───────────────────────────────────────┤
│     Interface Layer (iface/)          │  ← Motor, Sensor, Brake
├───────────────────────────────────────┤
│     Driver Layer (drv/)               │  ← PWM Motor, AS5600, Brake
├───────────────────────────────────────┤
│     Hardware Driver Layer (hwd/)      │  ← PWM, I2C, GPIO, Timer
├───────────────────────────────────────┤
│     Platform Layer (Board/STM32F411/) │  ← STM32 HAL implementations
└───────────────────────────────────────┘
```

**Benefits:**
- Logic (ctrl/, iface/, drv/) **does not depend** on microcontroller
- For porting to another platform — just change `Board/`
- Easy to test without hardware (mock HWD)

## Core Components

### Core Files
- `core.h`/`.c` - Basic types, enums, structures
- `config.h` - Global configuration parameters

### Interface Layer (iface/)
- `motor.h`/`.c` - Unified motor interface
- `sensor.h`/`.c` - Position sensor interface
- `brake.h`/`.c` - Brake control interface

### Hardware Driver Layer (HWD)
- `hwd.h` - Single entry point
- `hwd_pwm.h` - PWM abstraction
- `hwd_i2c.h` - I2C abstraction
- `hwd_gpio.h` - GPIO abstraction
- `hwd_timer.h` - Timer abstraction
- `hwd_udp.h` - UDP abstraction (для емуляції на ПК)

### Control Layer (ctrl/)
- `servo.h`/`.c` - Main servo controller
- `pid.h`/`.c` - PID controller with anti-windup
- `safety.h`/`.c` - Safety system
- `traj.h`/`.c` - Trajectory generator
- `err.h`/`.c` - Error handling

### Driver Layer (drv/)
- `drv/motor/pwm.h`/`.c` - PWM DC motor driver
- `drv/motor/pwm_udp.h`/`.c` - UDP PWM DC motor driver (для емуляції)
- `drv/sensor/as5600.h`/`.c` - AS5600 magnetic encoder
- `drv/sensor/aeat9922.h`/`.c` - AEAT-9922 magnetic encoder
- `drv/sensor/sensor_udp.h`/`.c` - UDP sensor driver (для емуляції)
- `drv/brake/brake.h`/`.c` - Electronic brake driver
- `drv/brake/brake_udp.h`/`.c` - UDP brake driver (для емуляції)

## Emulation Components

### UDP Emulation Platform
- `Board/PC_Emulation/` - емуляційна платформа для ПК
- `Emulator/` - основний застосунок для емуляції
- `Emulator/udp_client.h/.c` - UDP клієнт для взаємодії з моделлю

### UDP Communication Protocol
- `UDP_MSG_TYPE_MOTOR_CMD` - команда двигуну (потужність)
- `UDP_MSG_TYPE_SENSOR_CMD` - запит даних сенсора
- `UDP_MSG_TYPE_BRAKE_CMD` - команда гальмам
- `UDP_MSG_TYPE_MOTOR_STATE` - стан двигуна від моделі
- `UDP_MSG_TYPE_SENSOR_STATE` - дані сенсора від моделі
- `UDP_MSG_TYPE_BRAKE_STATE` - стан гальм від моделі

### Brake UDP драйвер
- Відправляє: `bool engaged` - активувати/деактивувати гальма
- Отримує: `bool engaged, bool ready, uint32_t error_flags` - стан гальм від моделі

### Motor UDP драйвер
- Відправляє: `float power` (від -100.0 до +100.0, негативне значення = зворотній напрямок) + запит на отримання статусу
- Отримує: `float position, float velocity, float current, bool stalled, bool overcurrent, uint32_t error_flags` - стан двигуна від моделі

### Sensor UDP драйвер
- Відправляє: запит на отримання даних + підтвердження готовності
- Отримує: `float angle, float velocity, bool connected, uint32_t error_flags` - дані з сенсора від моделі

## API Structure

### Main Servo Controller API
```c
// Initialization
Servo_Status_t Servo_Init(Servo_Controller_t* servo,
                          const Servo_Config_t* config,
                          Motor_Interface_t* motor);

Servo_Status_t Servo_InitWithBrake(Servo_Controller_t* servo,
                                    const Servo_Config_t* config,
                                    Motor_Interface_t* motor,
                                    Brake_Driver_t* brake);

// Main loop (call periodically)
Servo_Status_t Servo_Update(Servo_Controller_t* servo);

// Control
Servo_Status_t Servo_SetPosition(Servo_Controller_t* servo, float position);
Servo_Status_t Servo_SetVelocity(Servo_Controller_t* servo, float velocity);
Servo_Status_t Servo_Stop(Servo_Controller_t* servo);
Servo_Status_t Servo_EmergencyStop(Servo_Controller_t* servo);

// State reading
float Servo_GetPosition(const Servo_Controller_t* servo);
float Servo_GetVelocity(const Servo_Controller_t* servo);
Servo_State_t Servo_GetState(const Servo_Controller_t* servo);
bool Servo_IsAtTarget(const Servo_Controller_t* servo);
```

### Status Codes
```c
typedef enum {
    SERVO_OK = 0x00,        // Operation successful
    SERVO_ERROR = 0x01,     // General error
    SERVO_BUSY = 0x02,      // Device busy
    SERVO_TIMEOUT = 0x03,   // Operation timeout
    SERVO_INVALID = 0x04,   // Invalid parameters
    SERVO_NOT_INIT = 0x05,  // Device not initialized
    SERVO_ERROR_NULL_PTR = 0x06  // Null pointer
} Servo_Status_t;
```

## Building and Integration

### For STM32 Platform
1. TIM3 - PWM for motor (Channel 1: PA6, Channel 2: PA7)
2. I2C1 - Sensor AS5600 (SCL: PB6, SDA: PB7)
3. TIM5 - Microsecond timer (32-bit)
4. GPIO - Brake control (e.g., PA8)

### For PC Emulation
**Include paths:**
```
../ServoLib/Inc
../ServoLib/Board/PC_Emulation
```

**Source files:**
```
ServoLib/Src/**/*.c               # All .c files from Src/
ServoLib/Board/PC_Emulation/*.c   # All .c files from PC_Emulation/
Emulator/*.c                      # Main application files
```

### Basic Usage Example (PC Emulation)
```c
#include "ctrl/servo.h"
#include "drv/motor/pwm_udp.h"
#include "drv/brake/brake_udp.h"
#include "drv/sensor/sensor_udp.h"

// Global variables
Servo_Controller_t servo;
PWM_Motor_UDP_Driver_t motor;
Brake_UDP_Driver_t brake;
Sensor_UDP_Driver_t sensor;

void Emulator_Init_Example(void)
{
    // 1. Initialize sensor driver
    Sensor_UDP_Create(&sensor);
    Sensor_UDP_Init(&sensor);

    // 2. Initialize motor driver
    PWM_Motor_UDP_Config_t motor_config = {
        .type = MOTOR_TYPE_DC_PWM,
        .interface_mode = PWM_MOTOR_INTERFACE_UDP
    };
    PWM_Motor_UDP_Create(&motor, &motor_config);

    Motor_Params_t motor_params = {
        .max_power = 100.0f,
        .min_power = 5.0f,
        .max_current = 2000,
        .invert_direction = false
    };
    PWM_Motor_UDP_Init(&motor, &motor_params);

    // 3. Initialize brake driver
    Brake_UDP_Config_t brake_config = {
        .timeout_ms = 100,
        .update_interval_ms = 10,
        .auto_engage_on_error = true,
        .error_threshold = 10
    };
    Brake_UDP_Create(&brake, &brake_config);
    Brake_UDP_Init(&brake);

    // 4. Configure servo controller
    Servo_Config_t servo_config = {
        .update_frequency = 1000.0f,  // 1 kHz
        .enable_brake = true,
        // ... other parameters (PID, safety, trajectory)
    };

    // 5. Initialize servo controller with UDP drivers
    Servo_InitWithBrake(&servo, &servo_config,
                        &motor.interface, &brake.driver);
}

void Emulator_Loop_Example(void)
{
    // Set target position to 90°
    Servo_SetPosition(&servo, 90.0f);

    // Main loop
    while (1) {
        // Update drivers before updating servo controller
        Sensor_UDP_Update(&sensor);
        PWM_Motor_UDP_Update(&motor);
        Brake_UDP_Update(&brake);

        // Update controller
        Servo_Update(&servo);

        // Check if target reached
        if (Servo_IsAtTarget(&servo)) {
            // Target reached!
        }

        // Small delay for PC simulation
        Sleep(1);  // 1 ms for 1 kHz update rate
    }
}
```

## Development Conventions

- **C99 standard** for code compatibility
- **Static memory allocation** (no malloc)
- **Layered architecture** with clear separation of concerns
- **Hardware abstraction** through HWD layer
- **Error checking** with Servo_Status_t return codes
- **Fail-safe design** for safety-critical components
- **UDP Communication** for PC emulation (IP-based networking)

## Project Structure
```
ServoLib/
├── Inc/                    # Header files
│   ├── core.h              # Basic types
│   ├── config.h            # Configuration
│   ├── iface/              # Interfaces (motor, sensor, brake)
│   ├── hwd/                # HWD abstractions (pwm, i2c, gpio, timer, udp)
│   ├── drv/                # Drivers (motor, sensor, brake) with UDP versions
│   ├── ctrl/               # Control (servo, pid, safety, traj)
│   └── util/               # Utilities (math, buf)
│
├── Src/                    # Implementations (structure matches Inc/)
│
├── Board/                  # Platform-specific code
│   ├── STM32F411/          # STM32F411 implementation
│   └── PC_Emulation/       # PC Emulation implementation
│
├── Emulator/               # PC Emulation application
│   ├── main.c              # Main emulation application
│   ├── udp_client.h/.c     # UDP client for communication
│   └── README.md           # Emulation documentation
│
├── Examples/               # Usage examples
├── Doc/                    # Documentation
└── *.md                    # Documentation files
```

## Hardware Requirements (for real hardware)

- **MCU:** STM32F411CEU6 (BlackPill) or compatible
- **Motor:** DC motor with gearbox, up to 2A
- **H-bridge:** L298N, TB6612 or compatible
- **Encoder:** AS5600 or AEAT-9922 magnetic encoder (optional)
- **Brakes:** Electromagnetic fail-safe brakes (optional)
- **Power:** Separate motor supply (6-24V)

## Emulation Requirements (for PC simulation)

- **UDP Connection:** Connection to mathematical model via UDP
- **Network:** Local or remote connection to motor model
- **Configuration:** IP and port settings for communication
- **Mathematical Model:** Separate process running motor simulation

## Key Technical Specifications

- **Control frequency:** 1 kHz (1 ms period)
- **PWM frequency:** Configurable up to 100 kHz (default 1 kHz)
- **PWM resolution:** 1000 steps (0.1%)
- **Encoder resolution:** Up to 18-bit (AEAT-9922) or 12-bit (AS5600)
- **Safety response time:** < 10 ms for emergency stop
- **Memory usage:** Static allocation < 10 KB
- **UDP Communication:** Real-time data exchange with motor model

## Porting to Other Platforms

1. Create new directory for your platform: `ServoLib/Board/STM32F407/`
2. Copy files from existing platform: `cp -r ServoLib/Board/STM32F411/* ServoLib/Board/STM32F407/`
3. Adapt `board_config.h` (pins, timers)
4. Implement HWD functions in Board/ for your platform
5. Rebuild project

**All logic (ctrl/, iface/, drv/) remains unchanged!**

## PC Emulation Usage

For PC emulation, create a connection to a mathematical model of the motor system:

1. Configure UDP settings in `Board/PC_Emulation/board_config.h`
2. Build the emulator with PC-specific HWD implementations
3. Ensure the mathematical model is running and listening on the configured IP/port
4. Run the emulator to communicate with the model via UDP