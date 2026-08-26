# Структура бібліотеки ServoLib

## Дерево каталогів

```
ServoLib/
├── Inc/                        # Заголовочні файли
│   ├── core.h                  # Основні типи, enum, структури
│   ├── config.h                # Константи алгоритмів (POSITION_ERROR_THRESHOLD тощо)
│   │
│   ├── hwd/                    # HWD абстракції (hardware-independent declarations)
│   │   ├── hwd.h               # Єдиний фасад для всіх HWD-функцій
│   │   ├── hwd_pwm.h           # PWM сигнали
│   │   ├── hwd_adc.h           # ADC (DMA scan mode)
│   │   ├── hwd_i2c.h           # I2C (IT continuous read)
│   │   ├── hwd_spi.h           # SPI
│   │   ├── hwd_timer.h         # Таймери та затримки
│   │   ├── hwd_gpio.h          # GPIO
│   │   └── hwd_uart.h          # UART блокуючий (debug apps)
│   │
│   ├── drv/                    # Апаратні драйвери
│   │   ├── motor/
│   │   │   ├── motor.h         # Універсальний інтерфейс + hardware callbacks
│   │   │   └── pwm.h           # PWM драйвер DC двигуна
│   │   ├── position/
│   │   │   ├── position.h      # Універсальний інтерфейс position sensor
│   │   │   ├── incremental_encoder.h  # Квадратурний EXTI X4 + IC таймер
│   │   │   ├── as5600.h        # AS5600 12-bit I2C магнітний енкодер
│   │   │   └── encoder_ukf.h   # UKF фільтр [θ,ω,α] для інкрементального енкодера
│   │   ├── brake/
│   │   │   ├── brake.h         # Універсальний інтерфейс + state machine
│   │   │   └── gpio_brake.h    # GPIO драйвер електромагнітних гальм
│   │   └── current/
│   │       ├── current.h       # Універсальний інтерфейс датчика струму
│   │       └── acs712.h        # ACS712T (ефект Хола, ADC)
│   │
│   ├── ctrl/                   # Логіка керування
│   │   ├── servo.h             # Головний контролер (Servo_Controller_t)
│   │   ├── cascade.h           # Каскадний PID (3 контури + FF + slew)
│   │   ├── pid.h               # Базовий PID регулятор
│   │   ├── pid_mgr.h           # Менеджер PID (legacy, не компілюється)
│   │   ├── safety.h            # Система безпеки (межі, струм, watchdog)
│   │   ├── traj.h              # Генератор траєкторій
│   │   └── time.h              # Periodic_Timer_t, Cb_Timer_t (multi-rate control loop)
│   │
│   ├── comm/                   # Комунікаційний протокол
│   │   ├── servo_protocol.h    # Wire-структури + msg_id + field constants
│   │   └── servo_comm.h        # Comm API (init, send, receive)
│   │
│   └── util/
│       └── derivative.h        # Чисельна похідна (velocity з position)
│
├── Src/                        # Реалізації
│   ├── core.c
│   ├── hwd/hwd.c
│   ├── drv/
│   │   ├── motor/motor.c, pwm.c
│   │   ├── position/position.c, incremental_encoder.c, as5600.c, encoder_ukf.c
│   │   ├── brake/brake.c, gpio_brake.c
│   │   └── current/current.c, acs712.c
│   ├── ctrl/
│   │   ├── servo.c, cascade.c, pid.c, pid_mgr.c (legacy)
│   │   ├── safety.c, traj.c, time.c
│   ├── comm/
│   │   ├── servo_comm.c
│   │   └── servo_protocol_desc.h  # Приватні дескриптори полів
│   └── util/derivative.c
│
├── Board/                      # Конфігурація плат (піни, макроси USE_*)
│   ├── STM32F411_OCM3/         # Основна плата BlackPill (єдине місце з MCU-залежністю)
│   │   ├── board_config.h      # Піни, периферія, макроси USE_*
│   │   ├── board.c             # Board_Init() — системний клок, периферія
│   │   ├── hwd_pwm.c           # TIM3 CH1 PWM
│   │   ├── hwd_adc.c           # ADC1 DMA scan mode
│   │   ├── hwd_i2c.c           # I2C1 IT continuous read
│   │   ├── hwd_spi.c           # SPI
│   │   ├── hwd_timer.c         # TIM5 мкс + SysTick мс
│   │   ├── hwd_gpio.c          # GPIO
│   │   ├── hwd_uart.c          # USART1 блокуючий (debug apps)
│   │   ├── hwd_uart_async.h/c  # USART1 DMA (USE_COMM_ASYNC)
│   │   └── hwd_crc32.c         # Апаратний CRC32 (weak override)
│   └── STM32F411_EncoderHub/   # 6-канальний концентратор енкодерів
│       ├── board_config.h
│       └── board.c
│
├── Lib/                        # Git submodules / FetchContent
│   ├── frame_codec/            # COBS framing + CRC32/MPEG-2
│   ├── packet_codec/           # msg_id | payload layer
│   └── ukf_mcu/                # UKF фільтр (encoder_ukf + current UKF)
│
├── Apps/                       # Цілі для збірки
│   ├── debug_encoder/          # Тест інкрементального енкодера
│   ├── debug_motor/            # Тест PWM двигуна
│   ├── debug_brake/            # Тест GPIO гальма
│   ├── debug_current/          # Тест ACS712 датчика струму
│   ├── servo_full/             # Повний сервопривід (Servo_Controller_t + Safety)
│   └── servo_basic/            # Каскадний PID напряму + async UART DMA comm
│
├── cmake/
│   ├── targets/STM32F411_OCM3.cmake          # DEVICE, BOARD_DIR, BOARD_SRCS
│   ├── targets/STM32F411_ENCODER_HUB.cmake   # Конфігурація EncoderHub
│   ├── toolchain/arm-none-eabi.cmake
│   ├── stm32.cmake             # genlink, MCU_FLAGS, stm32_add_executable
│   └── ServoLib.cmake          # SERVOLIB_UTIL/MOTOR/POSITION/CURRENT/BRAKE/CTRL/COMM/ALL
│
├── Doc/                        # Документація
├── configure.sh                # Інтерактивний вибір BOARD/APP/PROGRAMMER → .preset
├── build.sh                    # Читає .preset → cmake --build
├── flash.sh                    # Читає .preset → openocd flash
└── CMakeLists.txt
```

---

## Архітектура шарів

```
Apps/               ← main.c — ініціалізація + control loop
    ↓
ctrl/               ← Cascade PID, Safety, Trajectory, Servo (hardware-independent)
comm/               ← Async протокол (hardware-independent)
    ↓
drv/                ← Motor, Position, Brake, Current (universal interfaces + callbacks)
    ↓
hwd/ + Board/       ← HWD declarations + libopencm3 implementations (board_config.h, hwd_*.c)
Lib/                ← frame_codec, packet_codec, ukf_mcu (git submodules / FetchContent)
```

**Ключовий принцип:** `ctrl/`, `drv/`, `comm/` не мають жодних `#include <libopencm3/...>`. Тільки `Board/` знає про MCU.

> **Виняток:** `Src/drv/position/incremental_encoder.c` напряму включає `board_config.h` та `<libopencm3/stm32/exti.h>` — через залежність EXTI конфігурації від апаратури.

---

## Universal Interface Pattern (всі 4 типи драйверів)

| Компонент | Interface struct | Driver struct | Factory |
|-----------|-----------------|---------------|---------|
| Motor | `Motor_Interface_t` | `PWM_Motor_Driver_t` | `PWM_Motor_Create()` |
| Position | `Position_Sensor_Interface_t` | `Incremental_Encoder_Driver_t`, `AS5600_Driver_t` | `Incremental_Encoder_Create()`, `AS5600_Create()` |
| Brake | `Brake_Interface_t` | `GPIO_Brake_Driver_t` | `GPIO_Brake_Create()` |
| Current | `Current_Sensor_Interface_t` | `ACS712_Driver_t` | `ACS712_Create()` |

Driver struct має interface як **перше поле** — безпечний каст між pointer types.

---

## CMake групи файлів (`cmake/ServoLib.cmake`)

| Змінна | Вміст |
|--------|-------|
| `SERVOLIB_UTIL` | `core.c`, `hwd.c`, `derivative.c`, `time.c` |
| `SERVOLIB_MOTOR` | `motor.c`, `pwm.c` |
| `SERVOLIB_POSITION` | `position.c`, `incremental_encoder.c`, `as5600.c`, `encoder_ukf.c` |
| `SERVOLIB_CURRENT` | `current.c`, `acs712.c` |
| `SERVOLIB_BRAKE` | `brake.c`, `gpio_brake.c` |
| `SERVOLIB_CTRL` | `servo.c`, `cascade.c`, `pid.c`, `safety.c`, `traj.c` |
| `SERVOLIB_COMM` | `servo_comm.c` (frame_codec і packet_codec — через `target_link_libraries`) |
| `SERVOLIB_ALL` | UTIL + MOTOR + POSITION + CURRENT + BRAKE + CTRL |

`pid_mgr.c` не входить до жодної групи (legacy файл).
`SERVOLIB_POSITION` потребує `target_link_libraries(... ukf_mcu)` у CMakeLists застосунку.

---

## Апаратне підключення

### STM32F411_OCM3 (BlackPill)

| Функція | Пін | Периферія |
|---------|-----|-----------|
| PWM мотора | PA6 | TIM3 CH1 AF2 |
| DIR мотора | PA7 | GPIO OUT |
| Гальмо | PA8 | GPIO OUT (active LOW) |
| Енкодер A | PB6 | EXTI6 + TIM4 CH1 IC (AF2) |
| Енкодер B | PB4 | EXTI4 |
| Датчик струму | PA4 | ADC1 CH4 (DMA2 Stream0 Ch0) |
| UART TX/RX | PA9/PA10 | USART1 AF7 |
| UART async TX DMA | — | DMA2 Stream7 Ch4 |
| UART async RX DMA | — | DMA2 Stream2 Ch4 |
| LED | PC13 | GPIO OUT |
| Мікросекундний таймер | — | TIM2 32-bit (100 MHz / 100 = 1 MHz) |

### STM32F411_EncoderHub (6-канальний концентратор)

| Функція | Пін | Периферія |
|---------|-----|-----------|
| ENC0 A | PA0 | EXTI0 + TIM2 CH1 IC AF1 |
| ENC1 A | PA1 | EXTI1 + TIM2 CH2 IC AF1 |
| ENC2 A | PA2 | EXTI2 + TIM2 CH3 IC AF1 |
| ENC3 A | PA3 | EXTI3 + TIM2 CH4 IC AF1 |
| ENC4 A | PA6 | EXTI6 + TIM3 CH1 IC AF2 |
| ENC5 A | PA8 | EXTI8 + TIM1 CH1 IC AF1 |
| ENC0-3 B | PB4,PA5,PB7,PB9 | EXTI4,5,7,9 |
| ENC4-5 B | PB10, PA11 | EXTI10,11 |
| SPI2 slave | PB12-15 | AF5 |
| DRDY out | PB0 | GPIO OUT |
| UART debug TX/RX | PA9/PA10 | USART1 AF7, 115200 |
| LED | PC13 | GPIO OUT |
| Мікросекундний таймер | — | TIM2 32-bit (+ IC для ENC0-3) |
| UKF тригер 10 kHz | — | TIM5 |

---

**Версія:** актуальна на 2026-05
