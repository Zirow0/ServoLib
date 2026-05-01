# ServoLib

Модульна C-бібліотека для керування DC сервоприводами на STM32F4.
Побудована на 5-шаровій архітектурі з повною апаратною абстракцією.

**Платформа:** STM32F411CEU6 (BlackPill) + libopencm3

---

## Архітектура

```
Application (Apps/)
    ↓
Control Layer (ctrl/)        — PID, Safety, Trajectory, Servo
    ↓
Driver Layer (drv/)          — Motor, Position, Brake, Current
    ↓
HWD Layer (hwd/)             — PWM, SPI, I2C, GPIO, Timer, UART
    ↓
Platform Layer (Board/)      — STM32F411_OCM3 (libopencm3)
```

Логіка (`ctrl/`, `drv/`) не залежить від платформи. Для портування — лише змінити `Board/`.

---

## Апаратне підключення (STM32F411CEU6 BlackPill)

| Функція              | Пін   | Периферія        |
|----------------------|-------|------------------|
| PWM мотора           | PA6   | TIM3 CH1 (AF2)   |
| DIR мотора           | PA7   | GPIO OUT         |
| Гальмо               | PA8   | GPIO OUT         |
| Енкодер A            | PB6   | EXTI6 + TIM4 CH1 IC |
| Енкодер B            | PB4   | EXTI4            |
| UART TX              | PA9   | USART1 (AF7)     |
| UART RX              | PA10  | USART1 (AF7)     |
| LED                  | PC13  | GPIO OUT         |

**PWM:** 20 kHz, 1000 кроків роздільної здатності
**UART:** 115200 бод (відлагодження)

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

| Ціль            | Опис                                   |
|-----------------|----------------------------------------|
| `debug_encoder` | Тест інкрементального енкодера + UART  |
| `debug_motor`   | Тест двигуна PWM+DIR + UART            |
| `debug_brake`   | Тест гальма GPIO + UART                |
| `servo_full`    | Повний сервопривід                     |

---

## Компоненти

### Датчики положення
- **Інкрементальний квадратурний енкодер** — EXTI X4 + IC таймер (TIM4), 32-біт лічильник *(активний)*
- **AS5600** — 12-біт I2C магнітний енкодер *(доступний)*

### Двигун
- Режим **PWM + DIR** (один PWM канал + GPIO напрямку)
- Режим **Dual PWM** (два канали, H-bridge)
- Статистика: час роботи, запуски, помилки

### Гальмо
- GPIO fail-safe (активне при LOW за замовчуванням)
- Станова машина: ENGAGED → RELEASING → RELEASED → ENGAGING
- Налаштовувані часи переходів

### Керування
- PID з anti-windup
- Генератор траєкторій (лінійні, S-криві)
- Система безпеки (обмеження позиції, швидкості, струму)
- Аварійна зупинка < 10 мс

---

## Структура проекту

```
ServoLib/
├── Inc/                        # Заголовочні файли
│   ├── core.h                  # Базові типи та enum
│   ├── hwd/                    # HWD абстракції
│   ├── drv/                    # Драйвери (motor, position, brake, current)
│   ├── ctrl/                   # Керування (servo, pid, safety, traj)
│   └── util/                   # Утиліти (math, buf, derivative)
├── Src/                        # Реалізації
├── Board/
│   └── STM32F411_OCM3/         # libopencm3 платформа
│       ├── board_config.h
│       ├── board.c
│       └── hwd_*.c
├── Apps/                       # Застосунки для збірки
│   ├── debug_encoder/
│   ├── debug_motor/
│   ├── debug_brake/
│   └── servo_full/
├── cmake/
│   ├── targets/STM32F411_OCM3.cmake
│   ├── toolchain/arm-none-eabi.cmake
│   └── ServoLib.cmake
├── configure.sh                # Конфігурація CMake + LSP
├── build.sh                    # Збірка
├── flash.sh                    # Прошивка через OpenOCD
├── CMakeLists.txt
└── CMakePresets.json
```

---

## LSP (clangd)

Проект налаштовано для роботи clangd. Після `./configure.sh`:
- `compile_commands.json` симлінкується в корінь
- `.clangd` фільтрує ARM GCC прапори для clang
- `.clangd-stubs/` містить заглушки для Nix-специфічних заголовків

Для коректної роботи clangd потрібен `--query-driver`:
```
--query-driver=/path/to/arm-none-eabi-gcc
```

---

## Технічні характеристики

| Параметр                   | Значення                     |
|----------------------------|------------------------------|
| MCU                        | STM32F411CEU6 (BlackPill)    |
| Тактова частота            | 100 MHz (HSE 25 MHz + PLL)   |
| Частота PWM                | 20 kHz, 1000 кроків          |
| Частота контуру керування  | 1 kHz                        |
| Аварійна зупинка           | < 10 мс                      |
| Стандарт коду              | C99, статична пам'ять        |
| Бібліотека платформи       | libopencm3                   |

---

## Документація

- `Doc/structure.md` — детальна структура
- `Doc/technical_specifications.md` — технічна специфікація
- `Doc/BRAKE_DRIVER.md` — драйвер гальм
- `Templates/config_user_template.h` — шаблон конфігурації

---

**Організація:** КПІ ім. Ігоря Сікорського · **Ліцензія:** MIT
