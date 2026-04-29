# ─── cmake/stm32.cmake — спільна логіка збірки для STM32 + libopencm3 ────────
# Потребує змінних із файлу плати:
#   DEVICE    — ідентифікатор чипу для genlink.py (напр. "stm32f411ceu6")
#   BOARD_DIR — шлях до Board/<назва>/
#   BOARD_SRCS — список hwd_*.c файлів плати

# ─── libopencm3 ───────────────────────────────────────────────────────────────

if(DEFINED ENV{LIBOPENCM3_DIR})
    set(OCM3_DIR "$ENV{LIBOPENCM3_DIR}")
else()
    message(FATAL_ERROR
        "LIBOPENCM3_DIR не встановлено.\n"
        "Запустіть у nix-shell або встановіть вручну:\n"
        "  export LIBOPENCM3_DIR=/path/to/libopencm3")
endif()

# ─── genlink.py: автовизначення параметрів чипу ───────────────────────────────

find_package(Python3 REQUIRED COMPONENTS Interpreter)

foreach(_MODE DEFS CPU FPU FAMILY)
    execute_process(
        COMMAND ${Python3_EXECUTABLE}
                "${OCM3_DIR}/scripts/genlink.py"
                "${OCM3_DIR}/ld/devices.data"
                "${DEVICE}"
                ${_MODE}
        OUTPUT_VARIABLE GENLINK_${_MODE}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "%" "" GENLINK_${_MODE} "${GENLINK_${_MODE}}")
endforeach()

separate_arguments(GENLINK_DEFS_LIST UNIX_COMMAND "${GENLINK_DEFS}")

# ─── MCU прапорці ─────────────────────────────────────────────────────────────

set(MCU_FLAGS -mthumb -mcpu=${GENLINK_CPU})
if(GENLINK_FPU STREQUAL "soft")
    list(APPEND MCU_FLAGS -mfloat-abi=soft)
elseif(GENLINK_FPU MATCHES "^hard-(.+)$")
    list(APPEND MCU_FLAGS -mfloat-abi=hard -mfpu=${CMAKE_MATCH_1})
endif()

set(GENLINK_LIB "opencm3_${GENLINK_FAMILY}")

message(STATUS "Device : ${DEVICE}")
message(STATUS "CPU    : ${GENLINK_CPU}  FPU: ${GENLINK_FPU}")
message(STATUS "Library: ${GENLINK_LIB}")

# ─── Глобальні прапорці компіляції ────────────────────────────────────────────

add_compile_options(
    ${MCU_FLAGS}
    -std=c99
    -Wall -Wextra
    -Wimplicit-function-declaration
    -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
    -Wundef -Wshadow
    -fno-common -ffunction-sections -fdata-sections
)

add_link_options(
    ${MCU_FLAGS}
    -static
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

# ─── Автогенерація лінкер-скрипта ─────────────────────────────────────────────

set(GENERATED_LD "${CMAKE_BINARY_DIR}/${DEVICE}.ld")

add_custom_command(
    OUTPUT  "${GENERATED_LD}"
    COMMAND arm-none-eabi-cpp
            -P
            ${GENLINK_DEFS_LIST}
            -I "${OCM3_DIR}/ld"
            "${OCM3_DIR}/ld/linker.ld.S"
            -o "${GENERATED_LD}"
    DEPENDS "${OCM3_DIR}/ld/linker.ld.S"
            "${OCM3_DIR}/ld/devices.data"
    COMMENT "Generating linker script for ${DEVICE}"
)

add_custom_target(generate_ld DEPENDS "${GENERATED_LD}")

# ─── OpenOCD target config ────────────────────────────────────────────────────
# GENLINK_FAMILY = "stm32f4" → "target/stm32f4x.cfg"

set(OPENOCD_TARGET_CFG "target/${GENLINK_FAMILY}x.cfg")

# ─── stm32_add_executable(<name> <sources...>) ────────────────────────────────
# Збирає ELF, підключає include ServoLib + OCM3, лінкує opencm3.
# Генерує .bin, .hex, виводить розміри секцій.
# Додає flash target: cmake --build <dir> --target flash

function(stm32_add_executable TARGET)
    add_executable(${TARGET} ${ARGN})
    add_dependencies(${TARGET} generate_ld)

    target_include_directories(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/Inc
        ${CMAKE_SOURCE_DIR}/Board/${BOARD}
        ${OCM3_DIR}/include
    )

    target_compile_definitions(${TARGET} PRIVATE
        ${GENLINK_DEFS_LIST}
    )

    target_link_directories(${TARGET} PRIVATE
        ${OCM3_DIR}/lib
    )

    target_link_libraries(${TARGET} PRIVATE ${GENLINK_LIB} m c gcc nosys)

    target_link_options(${TARGET} PRIVATE
        -T${GENERATED_LD}
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.map
    )

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
                $<TARGET_FILE:${TARGET}>
                ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex
                $<TARGET_FILE:${TARGET}>
                ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
        VERBATIM
    )

    add_custom_target(flash
        DEPENDS ${TARGET}
        COMMAND ${CMAKE_SOURCE_DIR}/flash.sh
                "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex"
                "${OPENOCD_TARGET_CFG}"
        USES_TERMINAL
        COMMENT "Flashing ${TARGET} via OpenOCD"
    )
endfunction()
