# План інтеграції OpENer EtherNet/IP у ServoLib - ФІНАЛЬНА ВЕРСІЯ

**Дата:** 26 грудня 2024
**Гілка:** ethernetip-integration
**Версія OpENer:** 2.3.0
**Статус:** Узгоджено, готово до реалізації ✅

---

## Фінальні технічні рішення

### 1. Платформа
- ✅ **Тільки PC Emulation** (Windows MinGW + POSIX)
- ❌ STM32 відкладено на майбутнє
- **Обґрунтування:** Швидший прототип, легше тестування з VirtualActuator v2.0

---

### 2. Структури даних

#### 2.1 Output Assembly (Commands: Controller → ServoLib) - 8 bytes

```c
typedef struct __attribute__((packed)) {
    uint16_t control_word;      // 2 bytes: DS402 Control Word
    float    pwm_duty_percent;  // 4 bytes: -100.0 to +100.0
    uint8_t  mode_of_operation; // 1 byte: 128 = PWM mode (розширюваність)
    uint8_t  brake_command;     // 1 byte: 0=no action, 1=release, 2=engage
} EIP_Command_t;  // Total: 8 bytes
```

**Mode of Operation values:**
```c
#define DS402_MODE_PWM_DIRECT 128  // Vendor-specific: Direct PWM control
// Майбутні режими:
// #define DS402_MODE_PROFILE_POSITION 1
// #define DS402_MODE_VELOCITY 3
```

**Control Word bits (DS402 standard):**
```c
#define DS402_CW_SWITCH_ON          (1 << 0)
#define DS402_CW_ENABLE_VOLTAGE     (1 << 1)
#define DS402_CW_QUICK_STOP         (1 << 2)  // 1=normal, 0=quick stop
#define DS402_CW_ENABLE_OPERATION   (1 << 3)
#define DS402_CW_FAULT_RESET        (1 << 7)
#define DS402_CW_HALT               (1 << 8)
```

**Assembly ID:** 150 (Output Assembly)

---

#### 2.2 Input Assembly (Telemetry: ServoLib → Controller) - ДИНАМІЧНИЙ розмір

**Header (завжди присутній) - 8 bytes:**
```c
typedef struct __attribute__((packed)) {
    uint16_t status_word;       // 2 bytes: DS402 Status Word
    uint16_t warning_code;      // 2 bytes: Custom warning flags
    uint32_t error_code;        // 4 bytes: Custom error code
} EIP_Telemetry_Header_t;  // 8 bytes
```

**Status Word bits (DS402 standard):**
```c
#define DS402_SW_READY_TO_SWITCH_ON (1 << 0)
#define DS402_SW_SWITCHED_ON        (1 << 1)
#define DS402_SW_OPERATION_ENABLED  (1 << 2)
#define DS402_SW_FAULT              (1 << 3)
#define DS402_SW_VOLTAGE_ENABLED    (1 << 4)
#define DS402_SW_QUICK_STOP         (1 << 5)  // 0=quick stop active
#define DS402_SW_SWITCH_ON_DISABLED (1 << 6)
#define DS402_SW_WARNING            (1 << 9)
#define DS402_SW_TARGET_REACHED     (1 << 10)
```

**Опціональні поля (4 bytes кожне):**
```c
// Додаються динамічно на основі enabled sensors в config_user.h

int32_t  position_counts;       // 4 bytes (якщо position sensor enabled)
float    velocity_rad_s;        // 4 bytes (якщо velocity enabled)
float    motor_current_A;       // 4 bytes (якщо current sensor enabled)
float    motor_voltage_V;       // 4 bytes (якщо voltage sensor enabled)
float    winding_temp_C;        // 4 bytes (якщо temperature sensor enabled)
```

**Розмір телеметрії:**
- Мінімальний: 8 bytes (header only)
- Максимальний: 28 bytes (header + 5 sensors)

**Assembly ID:** 100 (Input Assembly)

**Structure Discovery (Explicit Messaging, TCP 44818):**

Controller може запитати структуру через:
```
GET_ATTRIBUTE_SINGLE
  Class: 0x04 (Assembly Object)
  Instance: 100 (Input Assembly)
  Attribute: 3 (Assembly Descriptor)
```

Response:
```c
typedef struct {
    uint16_t total_size;        // Total assembly size in bytes
    uint8_t  field_count;       // Number of fields
    struct {
        uint16_t offset;        // Offset from start (bytes)
        uint8_t  size;          // Field size (bytes)
        uint8_t  type;          // 0=uint16, 1=int32, 2=float
        char     name[16];      // Field name (e.g., "position_counts")
    } fields[];
} Assembly_Descriptor_t;
```

**Приклад descriptor для full configuration:**
```json
{
  "total_size": 28,
  "field_count": 6,
  "fields": [
    {"offset": 0,  "size": 2, "type": "uint16", "name": "status_word"},
    {"offset": 2,  "size": 2, "type": "uint16", "name": "warning_code"},
    {"offset": 4,  "size": 4, "type": "uint32", "name": "error_code"},
    {"offset": 8,  "size": 4, "type": "int32",  "name": "position_counts"},
    {"offset": 12, "size": 4, "type": "float",  "name": "velocity_rad_s"},
    {"offset": 16, "size": 4, "type": "float",  "name": "motor_current_A"},
    {"offset": 20, "size": 4, "type": "float",  "name": "motor_voltage_V"},
    {"offset": 24, "size": 4, "type": "float",  "name": "winding_temp_C"}
  ]
}
```

---

#### 2.3 Config Assembly - НЕ ВИКОРИСТОВУЄТЬСЯ

- **Assembly ID:** 151 (reserved, але порожній)
- **Конфігурація:** Тільки через `config_user.h` (compile-time)
- **Обґрунтування:** Простіша реалізація, достатньо для поточних потреб

---

### 3. DS402 State Machine - ПОВНА РЕАЛІЗАЦІЯ

**State diagram:**
```
Power On
   │
   ▼
┌─────────────────────────────────────┐
│ 0. Not Ready to Switch On           │ (Initialization)
│    Status: 0x0000                   │
└────────────┬────────────────────────┘
             │ Automatic transition
             ▼
┌─────────────────────────────────────┐
│ 1. Switch On Disabled                │
│    Status: 0x0040                   │ ◄──┐
└────────────┬────────────────────────┘    │
             │ CW: Shutdown (bit 0=0→1)     │ CW: Disable Voltage
             ▼                              │
┌─────────────────────────────────────┐    │
│ 2. Ready to Switch On                │    │
│    Status: 0x0021                   │    │
└────────────┬────────────────────────┘    │
             │ CW: Switch On (bit 0,1=1)    │
             ▼                              │
┌─────────────────────────────────────┐    │
│ 3. Switched On                       │    │
│    Status: 0x0023                   │    │
└────────────┬────────────────────────┘    │
             │ CW: Enable Op (bit 0,1,3=1)  │
             ▼                              │
┌─────────────────────────────────────┐    │
│ 4. Operation Enabled ★               │    │
│    Status: 0x0027                   │ ◄──┼── Motor працює
└────────────┬────────────────────────┘    │
             │ Fault OR Quick Stop         │
             ▼                              │
┌─────────────────────────────────────┐    │
│ 5. Quick Stop Active                 │    │
│    Status: 0x0007                   │ ───┘
└─────────────────────────────────────┘
             │ Fault occurs
             ▼
┌─────────────────────────────────────┐
│ 6. Fault Reaction Active             │
│    Status: 0x000F                   │
└────────────┬────────────────────────┘
             │ Automatic transition
             ▼
┌─────────────────────────────────────┐
│ 7. Fault                             │
│    Status: 0x0008                   │
└────────────┬────────────────────────┘
             │ CW: Fault Reset (bit 7=1)
             └──────────► State 1 (Switch On Disabled)
```

**Реалізація:**

```c
// Inc/comm/eip_ds402.h

typedef enum {
    DS402_STATE_NOT_READY_TO_SWITCH_ON = 0,
    DS402_STATE_SWITCH_ON_DISABLED = 1,
    DS402_STATE_READY_TO_SWITCH_ON = 2,
    DS402_STATE_SWITCHED_ON = 3,
    DS402_STATE_OPERATION_ENABLED = 4,
    DS402_STATE_QUICK_STOP_ACTIVE = 5,
    DS402_STATE_FAULT_REACTION_ACTIVE = 6,
    DS402_STATE_FAULT = 7
} DS402_State_t;

typedef struct {
    DS402_State_t current_state;
    uint32_t fault_code;
    bool motor_enabled;
} DS402_StateMachine_t;

// Обробка transition на основі Control Word
DS402_State_t DS402_ProcessControlWord(
    DS402_StateMachine_t* sm,
    uint16_t control_word,
    bool fault_condition
);

// Формування Status Word
uint16_t DS402_GetStatusWord(
    DS402_State_t state,
    bool warning,
    bool target_reached,
    bool motor_stalled
);
```

**Transition table (основні):**

| From State | Control Word | Condition | To State |
|------------|--------------|-----------|----------|
| 0 | - | Auto | 1 (Switch On Disabled) |
| 1 | Shutdown (0x06) | - | 2 (Ready to Switch On) |
| 2 | Switch On (0x07) | - | 3 (Switched On) |
| 3 | Enable Operation (0x0F) | - | 4 (Operation Enabled) |
| 4 | Disable Operation (0x07) | - | 3 (Switched On) |
| 4 | Quick Stop (0x02) | - | 5 (Quick Stop Active) |
| Any | - | Fault=true | 6 (Fault Reaction) → 7 (Fault) |
| 7 | Fault Reset (0x80) | Fault cleared | 1 (Switch On Disabled) |

**State history logging:** ❌ НЕ реалізовувати (за рішенням)

---

### 4. Timing Architecture

**Simulation frequency:** 1 kHz (1 ms period)
**Communication frequency:** 1 kHz (1 ms RPI)
**Ratio:** 1:1

**Обґрунтування:**
- 1 kHz - бажана робоча частота ServoLib на STM32F411
- Кожен simulation update = новий пакет даних
- Простіша синхронізація
- Достатня точність для servo control

**Main loop:**
```c
// Emulator/main.c

const uint32_t CONTROL_FREQ_HZ = 1000;
const uint32_t PERIOD_MS = 1;

while (running) {
    uint32_t cycle_start = HWD_Timer_GetMillis();

    // 1. Process EtherNet/IP network events
    NetworkHandlerProcessOnce();  // OpENer function

    // 2. Receive commands from Output Assembly
    EIP_Comm_ReceiveCommands(&command);

    // 3. Process DS402 state machine
    DS402_ProcessControlWord(&ds402_sm, command.control_word, fault_present);

    // 4. Apply commands if Operation Enabled
    if (ds402_sm.current_state == DS402_STATE_OPERATION_ENABLED) {
        if (command.mode_of_operation == DS402_MODE_PWM_DIRECT) {
            Motor_SetPower(&motor, command.pwm_duty_percent);
        }
        if (command.brake_command == 1) {
            Brake_Release(&brake);
        } else if (command.brake_command == 2) {
            Brake_Engage(&brake);
        }
    }

    // 5. Update servo controller @ 1 kHz
    Servo_Update(&servo);

    // 6. Pack telemetry into Input Assembly
    EIP_Comm_PackTelemetry(&telemetry, &servo, &ds402_sm);

    // 7. Send telemetry
    EIP_Comm_SendTelemetry(&telemetry);

    // 8. Process watchdog
    EIP_Comm_ProcessWatchdog();

    // 9. Sleep until next cycle
    uint32_t elapsed = HWD_Timer_GetMillis() - cycle_start;
    if (elapsed < PERIOD_MS) {
        HWD_Timer_DelayMs(PERIOD_MS - elapsed);
    }
}
```

---

### 5. Safety - Watchdog

**Timeout:** 100 ms
**Дія:** Emergency stop

```c
// Inc/comm/eip_comm.h
#define EIP_WATCHDOG_TIMEOUT_MS 100

typedef struct {
    uint32_t last_command_time_ms;
    bool connection_active;
    uint32_t timeout_count;
} EIP_Watchdog_t;

void EIP_Comm_ProcessWatchdog(EIP_Watchdog_t* wd) {
    uint32_t current_time = HWD_Timer_GetMillis();

    if (wd->connection_active) {
        if (current_time - wd->last_command_time_ms > EIP_WATCHDOG_TIMEOUT_MS) {
            // Connection timeout - EMERGENCY STOP
            Servo_EmergencyStop(&servo);
            Brake_Engage(&brake);

            // Set DS402 fault
            ds402_sm.current_state = DS402_STATE_FAULT;
            ds402_sm.fault_code = ERROR_HEARTBEAT_TIMEOUT;

            wd->connection_active = false;
            wd->timeout_count++;

            TRACE_ERROR("EIP Watchdog timeout! Emergency stop activated.");
        }
    }
}

void EIP_Comm_OnCommandReceived(EIP_Watchdog_t* wd) {
    wd->last_command_time_ms = HWD_Timer_GetMillis();
    wd->connection_active = true;
}
```

**Fault codes:**
```c
#define ERROR_HEARTBEAT_TIMEOUT     0x8001
#define ERROR_OVERCURRENT           0x8002
#define ERROR_OVERTEMPERATURE       0x8003
#define ERROR_ENCODER_FAULT         0x8004
#define ERROR_BRAKE_FAULT           0x8005
```

---

### 6. CMake Integration

**Варіант:** A - add_subdirectory()

```cmake
# Emulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(ServoLib_Emulator C)

# OpENer configuration
set(OPENER_INSTALL_AS_LIB ON CACHE BOOL "Build OpENer as library")
set(OPENER_TRACE_LEVEL "INFO" CACHE STRING "OpENer trace level")
set(OPENER_RT OFF CACHE BOOL "Real-time mode (not needed for emulation)")

# Add OpENer as subdirectory
add_subdirectory(${CMAKE_SOURCE_DIR}/../External/OpENer
                 ${CMAKE_BINARY_DIR}/OpENer_build)

# ServoLib Emulator executable
add_executable(ServoLib_Emulator
    main.c
    udp_client.c  # Буде видалено пізніше
    ../Src/comm/eip_comm.c
    ../Src/comm/eip_assembly.c
    ../Src/comm/eip_ds402.c
    ../Src/ctrl/servo.c
    ../Src/ctrl/pid.c
    # ... інші файли
)

target_include_directories(ServoLib_Emulator PRIVATE
    ../Inc
    ../External/OpENer/source/src
)

target_link_libraries(ServoLib_Emulator PRIVATE
    OpENer
    ws2_32  # Windows sockets
)

# Compiler flags
target_compile_options(ServoLib_Emulator PRIVATE
    -Wall -Wextra
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)
```

---

### 7. Файлова структура

**Нові файли:**
```
ServoLib/
├── Inc/
│   └── comm/
│       ├── eip_comm.h           # EtherNet/IP communication API
│       ├── eip_assembly.h       # Assembly Object management
│       └── eip_ds402.h          # DS402 state machine
├── Src/
│   └── comm/
│       ├── eip_comm.c           # OpENer wrapper
│       ├── eip_assembly.c       # Dynamic assembly packing/unpacking
│       └── eip_ds402.c          # DS402 implementation
├── Board/
│   └── PC_Emulation/
│       └── eip_platform_pc.c   # Network handler для PC
└── External/
    └── OpENer/                  # Git submodule ✅
```

**Файли для видалення (після завершення міграції):**
```
Inc/hwd/hwd_udp.h
Src/hwd/hwd_udp.c
Board/PC_Emulation/hwd_udp.c
Src/drv/motor/pwm_udp.c
Src/drv/sensor/sensor_udp.c
Src/drv/brake/brake_udp.c
Emulator/udp_client.c
Emulator/udp_client.h
```

---

## План реалізації (6 фаз)

### Фаза 1: OpENer integration (2-3 дні) ← СТАРТ ТУТ

**Завдання:**

1. **Створити базові header files:**
   - `Inc/comm/eip_comm.h` - Public API
   - `Inc/comm/eip_assembly.h` - Assembly structures
   - `Inc/comm/eip_ds402.h` - DS402 definitions

2. **Реалізувати основні структури:**
   ```c
   // eip_comm.h
   typedef struct {
       uint16_t vendor_id;
       uint16_t device_type;
       uint16_t product_code;
       uint32_t serial_number;
       char     product_name[32];
   } EIP_Identity_t;

   typedef struct {
       uint16_t explicit_port;    // 44818
       uint16_t implicit_port;    // 2222
       EIP_Identity_t identity;
   } EIP_Config_t;

   Servo_Status_t EIP_Comm_Init(const EIP_Config_t* config);
   Servo_Status_t EIP_Comm_Update(void);
   void EIP_Comm_Shutdown(void);
   ```

3. **Реалізувати OpENer callbacks (заглушки):**
   ```c
   // eip_comm.c
   EipStatus ApplicationInitialization(void) {
       // Create Assembly Objects
       return kEipStatusOk;
   }

   EipStatus AfterAssemblyDataReceived(CipInstance* instance) {
       // Handle received commands
       return kEipStatusOk;
   }

   EipBool8 BeforeAssemblyDataSend(CipInstance* instance) {
       // Prepare telemetry
       return true;
   }

   void HandleApplication(void) {
       // Main loop hook
   }
   ```

4. **Базова CMake інтеграція:**
   - Додати OpENer до build system
   - Перевірити компіляцію

**Критерій успіху:**
- ✅ Проект компілюється з OpENer
- ✅ Можна викликати `EIP_Comm_Init()` (навіть якщо тільки заглушка)

---

### Фаза 2: Assembly Objects (2 дні)

**Завдання:**

1. **Реалізувати динамічний Assembly layout:**
   ```c
   // eip_assembly.c
   typedef struct {
       bool enabled;
       uint16_t offset;
   } Sensor_Field_t;

   typedef struct {
       Sensor_Field_t position;
       Sensor_Field_t velocity;
       Sensor_Field_t current;
       Sensor_Field_t voltage;
       Sensor_Field_t temperature;
       uint16_t total_size;
       uint8_t field_count;
   } Assembly_Layout_t;

   void EIP_Assembly_BuildLayout(
       Assembly_Layout_t* layout,
       bool position_enabled,
       bool velocity_enabled,
       bool current_enabled,
       bool voltage_enabled,
       bool temperature_enabled
   );
   ```

2. **Реалізувати packing/unpacking:**
   ```c
   void EIP_Assembly_PackTelemetry(
       uint8_t* buffer,
       const EIP_Telemetry_Header_t* header,
       const Servo_State_t* state,
       const Assembly_Layout_t* layout
   );

   void EIP_Assembly_UnpackCommand(
       const uint8_t* buffer,
       EIP_Command_t* command
   );
   ```

3. **Реалізувати Assembly Descriptor для Explicit Messaging:**
   ```c
   // Відповідь на GET_ATTRIBUTE_SINGLE для Assembly descriptor
   EipStatus GetAssemblyDescriptor(
       uint16_t assembly_id,
       uint8_t* response_buffer,
       uint16_t* response_size
   );
   ```

4. **Створити Assembly Objects в OpENer:**
   ```c
   EipStatus ApplicationInitialization(void) {
       // Output Assembly (Commands)
       CreateAssemblyObject(150, g_output_assembly_data, 8);

       // Input Assembly (Telemetry) - dynamic size
       CreateAssemblyObject(100, g_input_assembly_data, layout.total_size);

       // Configure Connection Point
       ConfigureExclusiveOwnerConnectionPoint(0, 150, 100, 0);

       return kEipStatusOk;
   }
   ```

**Критерій успіху:**
- ✅ Assembly Objects створюються з динамічним розміром
- ✅ Можна pack/unpack дані
- ✅ Descriptor правильно формується

---

### Фаза 3: PC Platform (2 дні)

**Завдання:**

1. **Реалізувати platform layer для PC:**
   ```c
   // Board/PC_Emulation/eip_platform_pc.c

   int NetworkHandlerInitialize(void) {
       // Initialize Windows sockets (WSA) or POSIX sockets
       // Create TCP socket for port 44818
       // Create UDP socket for port 2222
       return 0;  // Success
   }

   void NetworkHandlerFinish(void) {
       // Close sockets
       // Cleanup
   }
   ```

2. **Інтегрувати з OpENer network handler:**
   - OpENer викликає `NetworkHandlerProcessOnce()` кожен цикл
   - Обробляти вхідні TCP/UDP пакети
   - Передавати в OpENer CIP layer

3. **Оновити Emulator/main.c:**
   ```c
   int main() {
       // Initialize EtherNet/IP
       EIP_Config_t eip_config = {
           .vendor_id = 65535,
           .device_type = 0x02,  // AC Drive
           .product_code = 1,
           .serial_number = 12345,
           .product_name = "ServoLib EtherNet/IP"
       };

       EIP_Comm_Init(&eip_config);

       // Main loop @ 1 kHz
       while (running) {
           EIP_Comm_Update();  // Process network + assemblies
           Servo_Update(&servo);
           HWD_Timer_DelayMs(1);
       }

       EIP_Comm_Shutdown();
       return 0;
   }
   ```

**Критерій успіху:**
- ✅ Emulator запускається
- ✅ OpENer слухає на TCP 44818 та UDP 2222
- ✅ Можна підключитися через Wireshark/EIP scanner tool

---

### Фаза 4: Drivers update (2-3 дні)

**Завдання:**

1. **Реалізувати DS402 state machine:**
   ```c
   // eip_ds402.c
   DS402_State_t DS402_ProcessControlWord(
       DS402_StateMachine_t* sm,
       uint16_t control_word,
       bool fault_condition
   ) {
       // Implement full state transition table
       // Handle all 8 states
       // Return new state
   }
   ```

2. **Інтегрувати команди з Servo:**
   ```c
   // eip_comm.c - AfterAssemblyDataReceived()
   EIP_Command_t cmd;
   EIP_Assembly_UnpackCommand(g_output_assembly_data, &cmd);

   // Process DS402 state machine
   DS402_ProcessControlWord(&g_ds402_sm, cmd.control_word, fault_present);

   // Apply commands if Operation Enabled
   if (g_ds402_sm.current_state == DS402_STATE_OPERATION_ENABLED) {
       if (cmd.mode_of_operation == DS402_MODE_PWM_DIRECT) {
           // Update target PWM
           g_target_pwm = cmd.pwm_duty_percent;
       }
   }
   ```

3. **Оновити Servo_Update():**
   ```c
   void Servo_Update(Servo_Controller_t* servo) {
       // Read target from EIP
       if (g_ds402_sm.motor_enabled) {
           Motor_SetPower(&servo->motor, g_target_pwm);
       } else {
           Motor_Stop(&servo->motor);
       }

       // Update control loops
       PID_Update(&servo->pid, ...);

       // Pack telemetry for EIP
       // (буде відправлено в BeforeAssemblyDataSend)
   }
   ```

4. **Реалізувати watchdog:**
   ```c
   void EIP_Comm_ProcessWatchdog(void) {
       // Check timeout
       // Emergency stop if needed
   }
   ```

5. **Видалити старі UDP драйвери:**
   - `pwm_udp.c` → видалити
   - `sensor_udp.c` → видалити
   - `brake_udp.c` → видалити
   - `hwd_udp.c` → видалити

**Критерій успіху:**
- ✅ Команди через EIP керують мотором
- ✅ Телеметрія відправляється назад
- ✅ DS402 state machine працює
- ✅ Watchdog спрацьовує при timeout
- ✅ Старий UDP код видалено

---

### Фаза 5: CMake finalization (1 день)

**Завдання:**

1. **Оптимізувати build configuration:**
   ```cmake
   # Conditional compilation для різних platforms
   if(WIN32)
       target_link_libraries(ServoLib_Emulator PRIVATE ws2_32)
   elseif(UNIX)
       target_link_libraries(ServoLib_Emulator PRIVATE pthread)
   endif()
   ```

2. **Додати build варіанти:**
   ```cmake
   option(ENABLE_EIP_TRACES "Enable OpENer traces" ON)
   option(BUILD_EIP_TESTS "Build EtherNet/IP unit tests" OFF)

   if(ENABLE_EIP_TRACES)
       target_compile_definitions(ServoLib_Emulator PRIVATE
           OPENER_TRACE_LEVEL=OPENER_TRACE_LEVEL_INFO
       )
   endif()
   ```

3. **Оновити build.bat:**
   ```bat
   @echo off
   cd build
   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
   mingw32-make -j4
   cd ..
   ```

**Критерій успіху:**
- ✅ `build.bat` працює без помилок
- ✅ Release build оптимізований
- ✅ Debug build має trace output

---

### Фаза 6: Testing з VirtualActuator v2.0 (2-3 дні)

**Завдання:**

1. **Unit tests для критичних компонентів:**
   ```c
   // tests/test_eip_assembly.c
   void test_assembly_pack_unpack(void) {
       // Test packing/unpacking
       EIP_Command_t cmd = {.pwm_duty_percent = 50.0f, ...};
       uint8_t buffer[8];
       EIP_Assembly_PackCommand(buffer, &cmd);

       EIP_Command_t cmd2;
       EIP_Assembly_UnpackCommand(buffer, &cmd2);

       assert(cmd2.pwm_duty_percent == 50.0f);
   }

   void test_ds402_state_transitions(void) {
       // Test state machine
       DS402_StateMachine_t sm = {.current_state = DS402_STATE_SWITCH_ON_DISABLED};

       DS402_ProcessControlWord(&sm, 0x06, false);  // Shutdown
       assert(sm.current_state == DS402_STATE_READY_TO_SWITCH_ON);

       DS402_ProcessControlWord(&sm, 0x07, false);  // Switch On
       assert(sm.current_state == DS402_STATE_SWITCHED_ON);

       DS402_ProcessControlWord(&sm, 0x0F, false);  // Enable Operation
       assert(sm.current_state == DS402_STATE_OPERATION_ENABLED);
   }
   ```

2. **Integration test з VirtualActuator:**

   **Сценарій 1: Basic connection**
   - Запустити VirtualActuator v2.0
   - Запустити ServoLib Emulator
   - Перевірити Forward Open через Wireshark
   - Перевірити Implicit I/O @ 1 kHz

   **Сценарій 2: Motor control**
   - Пройти DS402 state machine (Disabled → Operation Enabled)
   - Відправити PWM command (50%)
   - Перевірити що VirtualActuator отримав команду
   - Перевірити телеметрію (position, current, etc.)

   **Сценарій 3: Fault handling**
   - Викликати fault (overcurrent)
   - Перевірити що Status Word = Fault (0x0008)
   - Відправити Fault Reset
   - Перевірити повернення до Switch On Disabled

   **Сценарій 4: Watchdog timeout**
   - Зупинити відправку команд
   - Через 100ms перевірити Emergency Stop
   - Перевірити Fault state

3. **Performance testing:**
   - Перевірити jitter (має бути < 1 ms)
   - Перевірити CPU usage (< 10%)
   - Перевірити network bandwidth

**Критерій успіху:**
- ✅ Всі unit tests pass
- ✅ ServoLib підключається до VirtualActuator
- ✅ Real-time I/O @ 1 kHz stable
- ✅ DS402 state machine працює коректно
- ✅ Fault handling працює
- ✅ Watchdog спрацьовує

---

## Testing Strategy (Комбінований підхід C)

### Unit Tests (CppUTest framework)

**Компоненти для testing:**
1. `eip_assembly.c` - packing/unpacking
2. `eip_ds402.c` - state machine transitions
3. Dynamic layout generation

**Приклад:**
```c
// tests/test_eip.c
#include "CppUTest/TestHarness.h"
#include "comm/eip_assembly.h"

TEST_GROUP(EIP_Assembly) {
    void setup() {
        // Setup
    }

    void teardown() {
        // Cleanup
    }
};

TEST(EIP_Assembly, PackUnpackCommand) {
    EIP_Command_t cmd = {
        .control_word = 0x0F,
        .pwm_duty_percent = 75.5f,
        .mode_of_operation = 128,
        .brake_command = 1
    };

    uint8_t buffer[8];
    EIP_Assembly_PackCommand(buffer, &cmd);

    EIP_Command_t result;
    EIP_Assembly_UnpackCommand(buffer, &result);

    CHECK_EQUAL(0x0F, result.control_word);
    DOUBLES_EQUAL(75.5f, result.pwm_duty_percent, 0.01);
    CHECK_EQUAL(128, result.mode_of_operation);
    CHECK_EQUAL(1, result.brake_command);
}
```

### Integration Tests

**Test environment:**
- VirtualActuator v2.0 на localhost
- ServoLib Emulator на localhost
- Wireshark для моніторингу

**Test cases:** (описано у Фазі 6)

---

## EDS File (EtherNet/IP Device Descriptor)

OpENer може генерувати EDS автоматично, але можна налаштувати:

```ini
[Device]
VendorID=65535
DeviceType=2
ProductCode=1
Revision=1.0
ProductName="ServoLib EtherNet/IP Adapter"
CatalogName="ServoLib Servo Drive"

[Device Classification]
Class=0x02
; AC Drive / Servo Drive

[Assembly]
Assem100=Input,28,0x0000,,,,"Telemetry Assembly"
Assem150=Output,8,0x0000,,,,"Command Assembly"

[Connection Manager]
Connection1=
   0x04,0x2c,0x14,0x6c,0x2c,0x64,
   0,Assem150,Assem100,,,
   3,1,
```

---

## Bandwidth Analysis @ 1 kHz

**Per cycle (1 ms):**
- Command: 8 bytes
- Telemetry: 28 bytes max
- UDP header: 28 bytes × 2 = 56 bytes
- **Total per cycle:** 92 bytes

**Per second:**
- 92 bytes × 1000 Hz = 92 KB/s = 736 kbit/s

**Percentage of 100 Mbit Ethernet:** 0.7% ✅ (дуже мало)

---

## Додаткові інструменти

### 1. Wireshark з EtherNet/IP dissector

**Filter:**
```
enip || cip
tcp.port == 44818 || udp.port == 2222
```

**Що шукати:**
- Register Session (Explicit)
- Forward Open (Connection setup)
- Implicit I/O packets @ 1 kHz

### 2. Python testing tool

```python
from pycomm3 import LogixDriver

# Connect to ServoLib
with LogixDriver('192.168.1.100') as servo:
    # Read Identity
    identity = servo.get_plc_info()
    print(f"Device: {identity}")

    # Read telemetry (Input Assembly 100)
    telemetry = servo.read_tag('InputAssembly100')
    print(f"Status: {telemetry}")

    # Write command (Output Assembly 150)
    command = {
        'control_word': 0x0F,  # Enable Operation
        'pwm_duty': 50.0,
        'mode': 128,
        'brake': 1
    }
    servo.write_tag('OutputAssembly150', command)
```

---

## Troubleshooting

### Проблема: OpENer не компілюється

**Рішення:**
```bash
# Перевірити submodule
git submodule update --init --recursive

# Перевірити CMake версію
cmake --version  # Має бути >= 3.15

# Очистити build
rm -rf build && mkdir build && cd build
cmake -G "MinGW Makefiles" ..
```

### Проблема: Connection timeout

**Рішення:**
- Перевірити firewall (порти 44818, 2222)
- Перевірити IP адресу (localhost = 127.0.0.1)
- Wireshark: чи є TCP SYN packets?

### Проблема: Телеметрія не оновлюється

**Рішення:**
- Перевірити `BeforeAssemblyDataSend()` - чи викликається?
- Перевірити layout - чи правильний offset?
- Додати TRACE_INFO для debug

---

## Наступні кроки після завершення

### Після Фази 6:

1. **Документація:**
   - Оновити README.md
   - Написати USER_GUIDE.md для EtherNet/IP
   - Додати приклади підключення

2. **Optimization:**
   - Profiling для знаходження bottlenecks
   - Оптимізація packing/unpacking (можливо SIMD)

3. **STM32 Port (майбутнє):**
   - Використати OpENer STM32 platform
   - Інтегрувати з STM32 Ethernet HAL
   - Додати LwIP або FreeRTOS+TCP

4. **Додаткові features:**
   - Position/Velocity modes (крім PWM)
   - Trajectory planning через EIP
   - Multi-axis coordination

---

## Контрольний чеклист перед стартом Фази 1 ✅

- [x] Створено гілку `ethernetip-integration`
- [x] Додано OpENer submodule
- [x] Вивчено OpENer API
- [x] Узгоджено всі технічні рішення
- [x] Створено фінальний план
- [ ] **Готовий почати Фазу 1** ← ТИ ТУТ

---

**Готовий стартувати?** 🚀

Якщо так, то починаємо з:
1. Створення `Inc/comm/eip_comm.h`
2. Створення `Inc/comm/eip_assembly.h`
3. Створення `Inc/comm/eip_ds402.h`
4. Базова реалізація `Src/comm/eip_comm.c`

Чи є ще питання перед стартом?
