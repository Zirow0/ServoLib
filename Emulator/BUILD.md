# Збірка емулятора ServoLib

Цей документ містить інструкції для компіляції та запуску емулятора ServoLib на Windows з MinGW.

## Вимоги

### Windows (MinGW-w64)

1. **MinGW-w64** з GCC компілятором
   - Завантажити: https://www.mingw-w64.org/
   - Або через MSYS2: https://www.msys2.org/

2. **CMake** версії 3.10 або новіше
   - Завантажити: https://cmake.org/download/

3. **Make** (входить в MinGW або MSYS2)

### Linux

1. **GCC компілятор**
   ```bash
   sudo apt install build-essential
   ```

2. **CMake**
   ```bash
   sudo apt install cmake
   ```

## Швидкий старт (Windows MinGW)

### 1. Відкрити MinGW/MSYS2 термінал

### 2. Перейти в директорію емулятора
```bash
cd C:/project/c_lib/ServoLib/Emulator
```

### 3. Створити директорію для збірки
```bash
mkdir build
cd build
```

### 4. Згенерувати Makefile з CMake
```bash
cmake -G "MinGW Makefiles" ..
```

### 5. Компілювати проект
```bash
mingw32-make
```

Або для багатопоточної збірки:
```bash
mingw32-make -j4
```

### 6. Запустити емулятор
```bash
./ServoLib_Emulator.exe
```

## Альтернативні методи збірки

### Метод 1: Один крок (Windows)
```bash
cd Emulator
mkdir build && cd build
cmake -G "MinGW Makefiles" .. && mingw32-make -j4
./ServoLib_Emulator.exe
```

### Метод 2: З використанням скрипта (Windows)
```bash
cd Emulator
./build.bat
```

### Метод 3: CMake з Ninja (швидше)
```bash
mkdir build && cd build
cmake -G "Ninja" ..
ninja
./ServoLib_Emulator.exe
```

## Опції збірки

### Release збірка (оптимізована)
```bash
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make
```

### Debug збірка (з символами налагодження)
```bash
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
mingw32-make
```

### Вимкнути debug виведення
```bash
cmake -G "MinGW Makefiles" -DENABLE_DEBUG=OFF ..
mingw32-make
```

## Очищення збірки

### Очистити тільки об'єктні файли
```bash
mingw32-make clean
```

### Повне очищення
```bash
cd ..
rm -rf build
```

Або з використанням CMake цілі:
```bash
mingw32-make clean-all
```

## Запуск емулятора

### Базовий запуск
```bash
./ServoLib_Emulator.exe
```

### Запуск через CMake
```bash
mingw32-make run
```

### Запуск з виведенням у файл
```bash
./ServoLib_Emulator.exe > output.log 2>&1
```

## Структура проекту після збірки

```
Emulator/
├── build/                          # Директорія збірки
│   ├── CMakeFiles/                 # Службові файли CMake
│   ├── ServoLib_Emulator.exe       # Виконуваний файл
│   ├── Makefile                    # Згенерований Makefile
│   └── *.o                         # Об'єктні файли
├── CMakeLists.txt                  # Конфігурація CMake
├── BUILD.md                        # Цей файл
├── main.c                          # Головний файл емулятора
├── udp_client.c/.h                 # UDP клієнт
└── *.md                            # Документація
```

## Налагодження проблем

### Помилка: "cmake: command not found"
- Переконайтесь, що CMake встановлено та додано до PATH
- Windows: Додайте `C:\Program Files\CMake\bin` до PATH

### Помилка: "mingw32-make: command not found"
- Переконайтесь, що MinGW встановлено
- Додайте `C:\mingw-w64\bin` (або MSYS2 bin) до PATH

### Помилка: "undefined reference to `__imp_WSAStartup`"
- Це нормально - CMakeLists.txt автоматично додає `-lws2_32`
- Якщо помилка залишається, перевірте CMakeLists.txt

### Помилка компіляції через кодування
```bash
cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS="-finput-charset=UTF-8" ..
```

### Перевірка версії компілятора
```bash
gcc --version
cmake --version
mingw32-make --version
```

## Інформація про збірку

Для виведення детальної інформації про конфігурацію:
```bash
mingw32-make info
```

## Конфігурація проекту

### Використання власної конфігурації

1. Створіть файл конфігурації:
```bash
cp ../Templates/config_user_template.h ../YourProject/Inc/config_user.h
```

2. Редагуйте параметри в `config_user.h`

3. Додайте include path при компіляції:
```bash
cmake -G "MinGW Makefiles" -DCMAKE_C_FLAGS="-I../YourProject/Inc" ..
```

## Вимоги до математичної моделі

Емулятор очікує UDP зв'язок з математичною моделлю двигуна:
- **IP адреса сервера моделі:** 127.0.0.1 (localhost)
- **Порт сервера:** 8888
- **Порт клієнта:** 8889
- **Частота оновлення:** 1 кГц (1 мс період)

Конфігурація в `Board/PC_Emulation/board_config.h`:
```c
#define UDP_SERVER_IP           "127.0.0.1"
#define UDP_SERVER_PORT         8888
#define UDP_CLIENT_PORT         8889
#define UDP_TIMEOUT_MS          100
```

## Додаткові ресурси

- **Документація емулятора:** `ARCHITECTURE.md`, `TESTING.md`
- **Протокол UDP:** `UDP_PROTOCOL.md`
- **Тестові сценарії:** `TEST_SCENARIOS.md`
- **Документація бібліотеки:** `../CLAUDE.md`
