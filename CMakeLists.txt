cmake_minimum_required(VERSION 3.20)

# ─── Параметри вибору цілі ────────────────────────────────────────────────────
# Передаються через -D або через CMakePresets.json
set(BOARD "STM32F411_OCM3" CACHE STRING "Плата/контролер [STM32F411_OCM3]")
set(APP   ""               CACHE STRING "Застосунок для збірки (обов'язково)")

# Тулчейн має бути відомий до project() — задається або тут, або через пресет
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_SOURCE_DIR}/cmake/toolchain/arm-none-eabi.cmake"
        CACHE FILEPATH "Файл тулчейну" FORCE)
endif()

project(ServoLib_${BOARD} C ASM)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Перевірки ────────────────────────────────────────────────────────────────

if(NOT APP)
    message(FATAL_ERROR
        "APP не вказано.\n"
        "Використання: cmake -DBOARD=STM32F411_OCM3 -DAPP=<назва> ...\n"
        "Або:          cmake --preset <назва_пресету>\n\n"
        "Доступні застосунки (Apps/):\n"
        "  debug_encoder  — тільки датчик положення\n"
        "  debug_motor    — тільки двигун\n"
        "  debug_brake    — тільки гальмо\n"
        "  servo_full     — повний сервопривід\n")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/targets/${BOARD}.cmake")
    message(FATAL_ERROR "Невідома плата: ${BOARD}. Додайте cmake/targets/${BOARD}.cmake")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Apps/${APP}")
    message(FATAL_ERROR "Застосунок '${APP}' не знайдено в Apps/")
endif()

# ─── Завантаження конфігурації плати, спільної логіки та джерел бібліотеки ────

include(cmake/targets/${BOARD}.cmake)   # → DEVICE, BOARD_DIR, BOARD_SRCS
include(cmake/stm32.cmake)              # → genlink, MCU_FLAGS, stm32_add_executable
include(cmake/ServoLib.cmake)           # → SERVOLIB_* групи файлів

# ─── LSP: compile_commands.json та .clangd ────────────────────────────────────

file(CREATE_LINK
    "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    SYMBOLIC
)

file(WRITE "${CMAKE_SOURCE_DIR}/.clangd"
"CompileFlags:\n"
"  Add:\n"
"    - --target=arm-none-eabi\n"
"  Remove:\n"
"    - -mfpu=*\n"
"    - -mfloat-abi=*\n"
"    - -mcpu=*\n"
"    - -mthumb\n"
)

# ─── Застосунок ───────────────────────────────────────────────────────────────

add_subdirectory(Apps/${APP})
