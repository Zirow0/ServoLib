# План інтеграції OpENer EtherNet/IP у ServoLib

**Дата:** 26 грудня 2024
**Гілка:** ethernetip-integration
**Версія OpENer:** 2.3.0

---

## 1. Огляд OpENer

**OpENer** - це open-source EtherNet/IP stack для I/O adapter пристроїв (slave devices).

### Ключові характеристики:

- **Мова:** Pure C (portable)
- **Ліцензія:** BSD (безкоштовна для комерційного використання)
- **Підтримка платформ:** STM32, POSIX (Linux), Windows (MinGW, Visual Studio)
- **Розробник:** Vienna University of Technology + EIP Stack Group
- **Репозиторій:** https://github.com/EIPStackGroup/OpENer
- **Документація:** https://eipstackgroup.github.io/OpENer/

### Функціонал:

✅ **Explicit Messaging** (TCP port 44818):
- Get/Set Attribute Single
- Forward Open/Close
- CIP Identity Object
- Configuration and diagnostics

✅ **Implicit I/O** (UDP):
- Producer-Consumer model
- Real-time циклічний обмін даними
- Configurable RPI (Requested Packet Interval)

✅ **CIP Objects:**
- Identity Object (Class 0x01)
- Assembly Object (Class 0x04)
- Connection Manager (Class 0x06)
- TCP/IP Interface (Class 0xF5)
- Ethernet Link (Class 0xF6)

✅ **DS402 Compatibility:**
- Може реалізувати DS402 Control/Status Words
- Assembly Objects для команд/телеметрії

---

## 2. Архітектура OpENer

```
OpENer Stack
├── CIP Layer (cip/)
│   ├── cipcommon.c/h         - Базові CIP функції
│   ├── cipidentity.c/h       - Identity Object
│   ├── cipassembly.c/h       - Assembly Object (I/O data)
│   ├── cipconnectionmanager.c/h - Connection lifecycle
│   └── ciptypes.h            - CIP data types
├── EtherNet/IP Encapsulation (enet_encap/)
│   ├── encap.c/h             - TCP/UDP encapsulation
│   └── endianconv.h          - Byte order conversion
├── Platform Abstraction (ports/)
│   ├── STM32/                - STM32 HAL implementation
│   ├── POSIX/                - Linux/Unix implementation
│   ├── MINGW/                - Windows MinGW
│   └── WIN32/                - Windows Visual Studio
└── API (opener_api.h)        - Public interface
```

**Ключовий файл:** `source/src/opener_api.h` - всі публічні функції для інтеграції

---

## 3. Приклад використання (з sample_application.c)

### 3.1 Ініціалізація

```c
#include "opener_api.h"
#include "appcontype.h"

// Визначити Assembly Objects
#define INPUT_ASSEMBLY_NUM  100   // Telemetry (device → controller)
#define OUTPUT_ASSEMBLY_NUM 150   // Commands (controller → device)
#define CONFIG_ASSEMBLY_NUM 151   // Configuration

// Буфери для даних
EipUint8 g_input_data[32];   // Telemetry buffer
EipUint8 g_output_data[32];  // Command buffer
EipUint8 g_config_data[10];  // Config buffer

EipStatus ApplicationInitialization(void) {
    // Створити Assembly Objects
    CreateAssemblyObject(INPUT_ASSEMBLY_NUM, g_input_data, sizeof(g_input_data));
    CreateAssemblyObject(OUTPUT_ASSEMBLY_NUM, g_output_data, sizeof(g_output_data));
    CreateAssemblyObject(CONFIG_ASSEMBLY_NUM, g_config_data, sizeof(g_config_data));

    // Налаштувати Connection Points
    ConfigureExclusiveOwnerConnectionPoint(0, OUTPUT_ASSEMBLY_NUM,
                                          INPUT_ASSEMBLY_NUM,
                                          CONFIG_ASSEMBLY_NUM);

    return kEipStatusOk;
}
```

### 3.2 Обробка отриманих команд

```c
EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    switch (instance->instance_number) {
        case OUTPUT_ASSEMBLY_NUM:
            // Розпакувати команди з g_output_data
            // Наприклад, DS402 control word, PWM duty, brake command
            float pwm_duty = *(float*)&g_output_data[0];
            uint16_t control_word = *(uint16_t*)&g_output_data[4];

            // Застосувати до ServoLib
            Servo_SetPower(&servo, pwm_duty);
            break;
    }
    return kEipStatusOk;
}
```

### 3.3 Підготовка телеметрії для відправки

```c
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    if (instance->instance_number == INPUT_ASSEMBLY_NUM) {
        // Запакувати телеметрію у g_input_data
        *(int32_t*)&g_input_data[0] = encoder_position;
        *(float*)&g_input_data[4] = motor_velocity;
        *(float*)&g_input_data[8] = motor_current;
        // ...
    }
    return true;  // Data is new
}
```

---

## 4. Інтеграція з ServoLib - Стратегія

### 4.1 Архітектурні рішення

**Варіант обраний:** Повна заміна UDP на EtherNet/IP (Варіант 2)

**Структура:**

```
ServoLib/
├── Inc/
│   └── comm/
│       ├── eip_comm.h              // EtherNet/IP communication interface
│       └── eip_ds402.h             // DS402 Control/Status structures
├── Src/
│   └── comm/
│       ├── eip_comm.c              // OpENer wrapper implementation
│       └── eip_assembly.c          // Assembly Object management
├── Board/
│   ├── STM32F411/
│   │   └── eip_platform_stm32.c   // STM32 network layer
│   └── PC_Emulation/
│       └── eip_platform_pc.c      // PC sockets for emulation
└── External/
    └── OpENer/                     // Git submodule (вже додано)
```

### 4.2 Видалення старого UDP коду

**Файли для видалення:**
- `Inc/hwd/hwd_udp.h`
- `Src/hwd/hwd_udp.c`
- `Board/PC_Emulation/hwd_udp.c`
- `drv/motor/pwm_udp.c`
- `drv/sensor/sensor_udp.c`
- `drv/brake/brake_udp.c`

**Файли для заміни:**
- `drv/motor/pwm_udp.c` → `drv/motor/motor_eip.c`
- `drv/sensor/sensor_udp.c` → `drv/sensor/sensor_eip.c`
- `drv/brake/brake_udp.c` → `drv/brake/brake_eip.c`

---

## 5. План реалізації (Фази)

### Фаза 1: Базова інтеграція OpENer (2-3 дні)

**Завдання:**
1. ✅ Додати OpENer як git submodule (ВИКОНАНО)
2. Створити CMake інтеграцію для OpENer
3. Реалізувати `eip_comm.c` - обгортка над OpENer API
4. Створити DS402-compatible структури даних

**Критерій успіху:** OpENer компілюється разом з ServoLib Emulator

---

### Фаза 2: Assembly Objects для ServoLib (2 дні)

**Завдання:**
1. Визначити формат Command Assembly (Output):
   ```c
   typedef struct __attribute__((packed)) {
       uint16_t control_word;      // DS402 Control Word
       float    pwm_duty_percent;  // -100.0 to +100.0
       uint8_t  mode_of_operation; // 128 = Direct PWM mode
       uint8_t  brake_command;     // 0=no action, 1=release, 2=engage
   } EIP_Command_t;  // Total: 8 bytes
   ```

2. Визначити формат Telemetry Assembly (Input):
   ```c
   typedef struct __attribute__((packed)) {
       uint16_t status_word;       // DS402 Status Word
       uint16_t warning_code;
       uint32_t error_code;
       int32_t  position_counts;   // Encoder position
       float    velocity_rad_s;
       float    motor_current_A;
       float    motor_voltage_V;
       float    winding_temp_C;
   } EIP_Telemetry_t;  // Total: 28 bytes
   ```

3. Реалізувати `ApplicationInitialization()` для ServoLib
4. Реалізувати `AfterAssemblyDataReceived()` та `BeforeAssemblyDataSend()`

**Критерій успіху:** Assembly Objects створюються, дані можна читати/писати

---

### Фаза 3: Platform Layer для PC Emulation (2 дні)

**Завдання:**
1. Реалізувати `eip_platform_pc.c`:
   - Network initialization (TCP 44818, UDP 2222)
   - Socket management
   - Integration з OpENer's `NetworkHandler`

2. Оновити `Emulator/main.c`:
   - Замінити UDP_Client на OpENer stack
   - Викликати `CipStackInit()`, `NetworkHandlerProcessOnce()`
   - Інтеграція з existing servo loop

**Критерій успіху:** Emulator запускається, OpENer слухає на портах 44818/2222

---

### Фаза 4: Оновлення драйверів (2-3 дні)

**Завдання:**
1. Створити `drv/motor/motor_eip.c`:
   - Розпаковує команди з Output Assembly
   - Викликає `PWM_Motor_SetPower()`
   - Пакує стан у Input Assembly

2. Аналогічно для `sensor_eip.c`, `brake_eip.c`

3. Видалити всі `*_udp.c` файли

**Критерій успіху:** Драйвери працюють через EtherNet/IP

---

### Фаза 5: CMake Build System (1 день)

**Завдання:**
1. Оновити `Emulator/CMakeLists.txt`:
   ```cmake
   add_subdirectory(../External/OpENer OpENer_build)
   target_link_libraries(ServoLib_Emulator PRIVATE OpENer)
   ```

2. Налаштувати OpENer build options:
   ```cmake
   set(OPENER_TRACE_LEVEL "INFO")
   set(OPENER_RT OFF)  # No real-time for emulation
   ```

**Критерій успіху:** `build.bat` компілює ServoLib + OpENer без помилок

---

### Фаза 6: Тестування з VirtualActuator v2.0 (2-3 дні)

**Завдання:**
1. Запустити VirtualActuator v2.0 (має бути готовий окремо)
2. З'єднати ServoLib Emulator з VirtualActuator через EtherNet/IP
3. Тестувати:
   - Forward Open/Close connection
   - Real-time I/O @ 1 kHz
   - DS402 state machine
   - Emergency stop, fault handling

**Критерій успіху:** Servo controller працює через EtherNet/IP

---

### Фаза 7: STM32 Platform (опціонально, 3-5 днів)

**Завдання:**
1. Використати OpENer STM32 port
2. Інтегрувати з STM32 Ethernet HAL або LwIP
3. Налаштувати real-time constraints

**Критерій успіху:** ServoLib працює на реальному STM32 з Ethernet

---

## 6. Ключові API функції OpENer

### 6.1 Initialization

```c
// Initialize CIP stack
EipStatus CipStackInit(CipUint16 unique_connection_id);

// Set device identity
void SetDeviceSerialNumber(EipUint32 serial_number);
void SetDeviceType(EipUint16 type);           // e.g., 0x02 = AC Drive
void SetDeviceProductCode(EipUint16 code);
void SetDeviceRevision(EipUint8 major, EipUint8 minor);
void SetDeviceVendorId(CipUint vendor_id);    // 65535 for testing
```

### 6.2 Assembly Management

```c
// Create Assembly Object
CipInstance* CreateAssemblyObject(EipUint32 instance_number,
                                  EipUint8* data,
                                  EipUint16 data_length);

// Configure Connection Points
EipStatus ConfigureExclusiveOwnerConnectionPoint(
    EipUint16 connection_number,
    EipUint16 output_assembly,
    EipUint16 input_assembly,
    EipUint16 config_assembly
);
```

### 6.3 Application Callbacks (потрібно реалізувати)

```c
// Called once during initialization
EipStatus ApplicationInitialization(void);

// Called when Output Assembly data received
EipStatus AfterAssemblyDataReceived(CipInstance* instance);

// Called before Input Assembly data sent
EipBool8 BeforeAssemblyDataSend(CipInstance* instance);

// Called on each main loop iteration
void HandleApplication(void);

// Called on I/O connection events
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                           unsigned int input_assembly_id,
                           IoConnectionEvent io_connection_event);
```

### 6.4 Network Processing

```c
// Main network processing function (call in main loop)
EipStatus NetworkHandlerProcessOnce(void);

// Initialize platform network
int NetworkHandlerInitialize(void);

// Shutdown network
void NetworkHandlerFinish(void);
```

---

## 7. Структури даних DS402

### DS402 Control Word (біти)

```c
#define DS402_CW_SWITCH_ON          (1 << 0)
#define DS402_CW_ENABLE_VOLTAGE     (1 << 1)
#define DS402_CW_QUICK_STOP         (1 << 2)   // 1=normal, 0=stop
#define DS402_CW_ENABLE_OPERATION   (1 << 3)
#define DS402_CW_FAULT_RESET        (1 << 7)
#define DS402_CW_HALT               (1 << 8)
```

### DS402 Status Word (біти)

```c
#define DS402_SW_READY_TO_SWITCH_ON (1 << 0)
#define DS402_SW_SWITCHED_ON        (1 << 1)
#define DS402_SW_OPERATION_ENABLED  (1 << 2)
#define DS402_SW_FAULT              (1 << 3)
#define DS402_SW_VOLTAGE_ENABLED    (1 << 4)
#define DS402_SW_QUICK_STOP         (1 << 5)   // 0=active
#define DS402_SW_SWITCH_ON_DISABLED (1 << 6)
#define DS402_SW_WARNING            (1 << 9)
#define DS402_SW_TARGET_REACHED     (1 << 10)
```

---

## 8. Конфігурація OpENer для ServoLib

### EDS файл (EtherNet/IP Device Descriptor)

OpENer генерує EDS файл автоматично, але можна налаштувати:

```
Vendor ID: 65535 (experimental)
Device Type: 0x02 (AC Drive / Servo)
Product Code: 1
Product Name: "ServoLib EtherNet/IP Adapter"
Revision: 1.0

Assemblies:
- Input (100): 28 bytes (telemetry)
- Output (150): 8 bytes (commands)
- Config (151): 0 bytes (unused)
```

### Timing Configuration

```c
// У ApplicationInitialization()
g_tcpip.encapsulation_inactivity_timeout = 120;  // seconds

// Connection RPI (Requested Packet Interval)
// Встановлюється Controller при Forward Open
// Типово: 1000 μs (1 kHz) для servo drives
```

---

## 9. Переваги та недоліки рішення

### Переваги:

✅ **Промисловий стандарт** - EtherNet/IP підтримується всіма major PLC виробниками
✅ **DS402 compatibility** - стандарт для servo drives
✅ **Інтероперабельність** - може працювати з реальними PLC (Allen-Bradley, Siemens, etc.)
✅ **Mature codebase** - OpENer в production з 2009 року
✅ **STM32 ready** - вже портований на STM32
✅ **Open source** - BSD ліцензія, безкоштовний для комерції

### Недоліки:

⚠️ **Складність** - EtherNet/IP складніший за простий UDP
⚠️ **Overhead** - TCP + UDP, більше мережевого трафіку
⚠️ **Learning curve** - потрібно розуміти CIP/DS402
⚠️ **Breaking change** - старий UDP код буде видалений

---

## 10. Альтернативні підходи

### Якщо OpENer виявиться надто складним:

**План B: Ручна реалізація Implicit I/O**

1. Реалізувати тільки UDP Implicit I/O (без OpENer)
2. Використовувати фіксовані структури (як VirtualActuator v2.0)
3. Додати OpENer пізніше для Explicit Messaging

**Переваги Plan B:**
- Швидше до working prototype
- Менше залежностей
- Простіше для debugging

**Недоліки Plan B:**
- Не повністю EtherNet/IP compliant
- Не працюватиме з реальними PLC
- Доведеться переписувати пізніше

---

## 11. Наступні кроки

**Immediate actions:**

1. ✅ Створити гілку `ethernetip-integration` (ВИКОНАНО)
2. ✅ Додати OpENer submodule (ВИКОНАНО)
3. ✅ Вивчити OpENer API (ВИКОНАНО)
4. ⏭️ Почати Фазу 1: CMake integration
5. ⏭️ Створити `eip_comm.h/c` базовий код

**Questions to resolve:**

- Чи потрібна підтримка STM32 Ethernet зараз, чи тільки PC emulation?
- Який Device Type використовувати? (0x02 = AC Drive, 0x2B = Generic Device)
- Чи використовувати Config Assembly, чи залишити його порожнім?

---

## 12. Ресурси

### Документація:

- **OpENer Docs:** https://eipstackgroup.github.io/OpENer/
- **OpENer GitHub:** https://github.com/EIPStackGroup/OpENer
- **ODVA (EtherNet/IP standard):** https://www.odva.org/
- **DS402 (CiA 402):** https://www.can-cia.org/can-knowledge/canopen/cia402/

### Mailing Lists:

- Developers: https://groups.google.com/forum/#!forum/eip-stack-group-opener-developers
- Users: https://groups.google.com/forum/#!forum/eip-stack-group-opener-users

### Tools:

- **Wireshark** з EtherNet/IP dissector - для debugging протоколу
- **pycomm3** (Python) - для тестування EtherNet/IP devices

---

**Документ підготував:** Claude (AI Assistant)
**Дата:** 26 грудня 2024
**Версія:** 1.0
