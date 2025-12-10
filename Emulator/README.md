# Емуляція сервоприводу через UDP

Цей проект дозволяє емулювати сервопривід на персональному комп'ютері через UDP зв'язок з математичною моделлю двигуна.

## Структура проекту

- `Board/PC_Emulation/` - емуляційна платформа з UDP драйверами
- `Emulator/` - основний застосунок для запуску емуляції
- `Inc/drv/*/` та `Src/drv/*/` - UDP версії драйверів (brake, motor, sensor)
- `Inc/hwd/` та `Src/hwd/` - HWD драйвери для емуляції

## Компоненти

### 1. Brake UDP драйвер
- Відправляє: `bool engaged` - активувати/деактивувати гальма
- Отримує: `bool engaged, bool ready, uint32_t error_flags` - стан гальм від моделі

### 2. Motor UDP драйвер
- Відправляє: `float power` (від -100.0 до +100.0, негативне значення = зворотній напрямок) + запит на отримання статусу
- Отримує: `float position, float velocity, float current, bool stalled, bool overcurrent, uint32_t error_flags` - стан двигуна від моделі

### 3. Sensor UDP драйвер
- Відправляє: запит на отримання даних + підтвердження готовності
- Отримує: `float angle, float velocity, bool connected, uint32_t error_flags` - дані з сенсора від моделі

## UDP Повідомлення

- `UDP_MSG_TYPE_MOTOR_CMD`: Команда для двигуна (power)
- `UDP_MSG_TYPE_SENSOR_CMD`: Запит даних сенсора
- `UDP_MSG_TYPE_BRAKE_CMD`: Команда для гальм (engaged)
- `UDP_MSG_TYPE_MOTOR_STATE`: Стан двигуна від моделі
- `UDP_MSG_TYPE_SENSOR_STATE`: Дані сенсора від моделі
- `UDP_MSG_TYPE_BRAKE_STATE`: Стан гальм від моделі

## Запуск емуляції

1. Запустіть математичну модель двигуна (окремий проект)
2. Запустіть цей емулятор командою:
```
make run-emulator
```
або
```
gcc -o emulator Emulator/main.c Emulator/udp_client.c Src/*.c Src/drv/**/*.c Board/PC_Emulation/*.c -IInc -IBoard/PC_Emulation -DUSE_REAL_HARDWARE=0
./emulator
```

## Налаштування

Всі налаштування знаходяться в `Board/PC_Emulation/board_config.h`:
- IP адреса та порти UDP зв'язку
- Таймаути та частоти оновлення
- Емуляційні параметри

## Тестування

Під час роботи емулятор виводить:
- Поточну позицію та швидкість
- Стан системи
- Помилки зв'язку

Дані оновлюються з частотою 1 кГц.