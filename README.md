# ServoLib

Модульна C-бібліотека для керування DC сервоприводами на STM32F4.
Побудована на 5-шаровій архітектурі з повною апаратною абстракцією.

**Платформа:** STM32F411CEU6 (BlackPill) + libopencm3

---

## Архітектура

```
Application (Apps/)
    ↓
Control Layer (ctrl/)        — Cascade PID, Safety, Trajectory, Servo
Comm Layer (comm/)           — Async протокол (frame + packet + CRC32)
    ↓
Driver Layer (drv/)          — Motor, Position, Brake, Current
    ↓
HWD Layer (hwd/)             — PWM, SPI, I2C, GPIO, Timer, UART
    ↓
Platform Layer (Board/)      — STM32F411_OCM3 (libopencm3)
External Libs (Lib/)         — frame_codec, packet_codec
```

Логіка (`ctrl/`, `drv/`, `comm/`) не залежить від платформи. Для портування — лише змінити `Board/`.

---

## Апаратне підключення (STM32F411CEU6 BlackPill)

| Функція              | Пін   | Периферія                    |
|----------------------|-------|------------------------------|
| PWM мотора           | PA6   | TIM3 CH1 (AF2)               |
| DIR мотора           | PA7   | GPIO OUT                     |
| Гальмо               | PA8   | GPIO OUT                     |
| Енкодер A            | PB6   | EXTI6 + TIM4 CH1 IC          |
| Енкодер B            | PB4   | EXTI4                        |
| Датчик струму        | PA4   | ADC1 CH4 (DMA2 Stream0)      |
| UART TX/RX           | PA9/PA10 | USART1 AF7                |
| LED                  | PC13  | GPIO OUT                     |

**UART async (DMA):** TX — DMA2 Stream7 Ch4, RX — DMA2 Stream2 Ch4 (circular + IDLE IRQ)

---

## Збірка

### Вимоги

- `arm-none-eabi-gcc`
- `cmake` ≥ 3.20, `ninja`
- `libopencm3` (змінна `LIBOPENCM3_DIR`)
- `openocd` (для прошивки)

### Конфігурація і збірка

```bash
# 1. Інтерактивно обрати плату, ціль і програматор
./configure.sh

# 2. Зібрати
./build.sh

# 3. Прошити
./flash.sh
```

### Доступні цілі

| Ціль            | Опис                                                       |
|-----------------|------------------------------------------------------------|
| `debug_encoder` | Тест інкрементального енкодера + блокуючий UART            |
| `debug_motor`   | Тест двигуна PWM+DIR + блокуючий UART                      |
| `debug_brake`   | Тест гальма GPIO + блокуючий UART                          |
| `servo_full`    | Повний сервопривід (Servo_Controller_t + Safety)           |
| `servo_basic`   | Каскадний PID напряму + async UART DMA комунікація         |

---

## Компоненти

### Каскадний PID (`ctrl/cascade`)

Три вкладені контури:

```
POS режим:  pos-PID(рад) → vel_sp → vel-PID(рад/с) → current_sp → trq-PID → %
VEL режим:                           vel-PID(рад/с) → current_sp → trq-PID → %
TRQ режим:                                             current_sp → trq-PID → %
```

Feedforward: `ff = ff_j * current_sp + ff_b * ω` (%)

- **Slew rate limiter** — обмежує `Δ%/с` команди двигуну
- **Anti-windup** — через `i_limit` або clamp відносно `out_min/max`
- **`Cascade_ApplyConfig()`** — оновлює коефіцієнти без скидання інтеграторів (online тюнінг)

### Датчики положення

- **Інкрементальний квадратурний енкодер** — EXTI X4 + IC таймер, 32-біт лічильник
- **AS5600** — 12-біт I2C магнітний енкодер, фоновий IT цикл

Всі одиниці: **радіани** / **рад/с**.

### Двигун

- Режим **PWM + DIR** (один PWM канал + GPIO напрямку)
- Режим **Dual PWM** (два канали, H-bridge: L298N, TB6612)

### Гальмо

- GPIO fail-safe (увімкнено при LOW за замовчуванням)
- Стан: `ENGAGED ↔ ENGAGING ↔ RELEASING ↔ RELEASED`

### Датчик струму

- **ACS712T** (ефект Хола) — 5A/20A/30A варіанти, EMA фільтр
- ADC1 scan+continuous + DMA2 circular (без CPU участі)

### Безпека (`ctrl/safety`)

- Обмеження позиції, швидкості, прискорення (рад, рад/с, рад/с²)
- Аварійний ліміт струму + таймаут
- Watchdog контуру керування
- Аварійна зупинка < 10 мс

---

## Async Comm (`servo_basic`, `USE_COMM_ASYNC`)

Повна асинхронна комунікація між STM32 та хостом по UART DMA.

### Стек протоколу

```
[data] → packet_encode(msg_id | payload) → frame_encode(COBS + CRC32) → UART DMA TX
UART DMA RX (circular) → IDLE IRQ → staging buf → frame_decode → packet_decode → dispatch
```

**CRC32:** апаратний блок STM32 (CRC-32/MPEG-2). ISR лише копіює байти — `frame_decode` з CRC тільки в main loop.

### Структури протоколу

| Структура | Розмір | Напрям | Призначення |
|-----------|--------|--------|-------------|
| `servo_telemetry_t` | 21 Б | STM32→хост | Базова телеметрія: θ, ω, I, target, mode |
| `servo_command_t` | 5 Б | хост→STM32 | Зміна режиму та setpoint |
| `cascade_telemetry_t` | 81 Б | STM32→хост | Повний знімок PID: сигнальний ланцюг + P/I/D терми + інтегратори |
| `cascade_config_t` | 84 Б | хост↔STM32 | Параметри каскаду: kp/ki/kd/limits для 3 контурів + FF + slew |

Підтримується **struct mode** (повна структура) та **param mode** (`[offset][type][value]` — один параметр).

### API

```c
// Ініціалізація
frame_crc32_init();
servo_comm_init(send_fn);
uart_init(&inst, &hw, baud, servo_comm_on_rx);

// Main loop
servo_comm_process_rx();                          // декодування + dispatch
servo_comm_get_command(&cmd);                     // прийом команди
servo_comm_get_cascade_config(&cfg);              // прийом конфігу → Cascade_ApplyConfig()

servo_comm_send_cascade(&telem);                  // TX 100 Hz
servo_comm_send_cascade_config(&cfg);             // підтвердження конфігу
```

---

## Структура проекту

```
ServoLib/
├── Inc/                        # Заголовочні файли
│   ├── core.h                  # Базові типи та enum
│   ├── hwd/                    # HWD абстракції
│   ├── drv/                    # Драйвери (motor, position, brake, current)
│   ├── ctrl/                   # Керування (servo, cascade, pid, safety, traj)
│   ├── comm/                   # Протокол (servo_protocol.h, servo_comm.h)
│   └── util/                   # Утиліти
├── Src/                        # Реалізації
│   ├── ctrl/
│   ├── drv/
│   └── comm/                   # servo_comm.c, servo_protocol_desc.h
├── Lib/                        # Зовнішні бібліотеки (git submodules)
│   ├── frame_codec/            # COBS framing + CRC32
│   └── packet_codec/           # msg_id | payload layer
├── Board/
│   └── STM32F411_OCM3/         # libopencm3 платформа
│       ├── board_config.h
│       ├── board.c
│       ├── hwd_*.c
│       ├── hwd_uart_async.h/c  # DMA UART (USE_COMM_ASYNC)
│       └── hwd_crc32.c         # Апаратний CRC32 override
├── Apps/
│   ├── debug_encoder/
│   ├── debug_motor/
│   ├── debug_brake/
│   ├── servo_full/
│   └── servo_basic/            # Каскадний PID + async comm
├── cmake/
│   ├── targets/STM32F411_OCM3.cmake
│   ├── toolchain/arm-none-eabi.cmake
│   └── ServoLib.cmake          # SERVOLIB_*, SERVOLIB_COMM
├── configure.sh
├── build.sh
├── flash.sh
├── CMakeLists.txt
└── CMakePresets.json
```

---

## LSP (clangd)

Після `./configure.sh APP=<ціль>`:
- `compile_commands.json` симлінкується в корінь
- `.clangd` фільтрує ARM GCC прапори для clang

---

## Технічні характеристики

| Параметр                   | Значення                     |
|----------------------------|------------------------------|
| MCU                        | STM32F411CEU6 (BlackPill)    |
| Тактова частота            | 100 MHz (HSE 25 MHz + PLL)   |
| Частота PWM                | 20 kHz, 1000 кроків          |
| Частота контуру керування  | 1 kHz                        |
| Частота телеметрії         | 100 Hz (DMA UART)            |
| Аварійна зупинка           | < 10 мс                      |
| Стандарт коду              | C99, статична пам'ять        |
| Бібліотека платформи       | libopencm3                   |

---

**Організація:** КПІ ім. Ігоря Сікорського · **Ліцензія:** MIT
