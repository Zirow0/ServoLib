# ─── STM32F411CEU6 + libopencm3 ──────────────────────────────────────────────

set(MCU_FLAGS -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)

add_compile_options(
    ${MCU_FLAGS}
    -std=c99
    -Wall -Wextra
    -Wimplicit-function-declaration
    -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
    -Wundef -Wshadow
    -fno-common -ffunction-sections -fdata-sections
    -DSTM32F4 -DSTM32F411xE
)

add_link_options(
    ${MCU_FLAGS}
    -static
    -nostartfiles
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

# ─── libopencm3 ───────────────────────────────────────────────────────────────

if(DEFINED ENV{LIBOPENCM3_DIR})
    set(OCM3_DIR "$ENV{LIBOPENCM3_DIR}")
else()
    message(FATAL_ERROR
        "LIBOPENCM3_DIR не встановлено.\n"
        "Запустіть у nix-shell або встановіть вручну:\n"
        "  export LIBOPENCM3_DIR=/path/to/libopencm3")
endif()

# ─── Board HWD sources ────────────────────────────────────────────────────────

set(BOARD_DIR ${CMAKE_SOURCE_DIR}/Board/STM32F411_OCM3)

set(BOARD_SRCS
    ${BOARD_DIR}/board.c
    ${BOARD_DIR}/hwd_pwm.c
    ${BOARD_DIR}/hwd_spi.c
    ${BOARD_DIR}/hwd_i2c.c
    ${BOARD_DIR}/hwd_gpio.c
    ${BOARD_DIR}/hwd_timer.c
)

set(LINKER_SCRIPT ${BOARD_DIR}/STM32F411CE.ld)

# ─── stm32_add_executable(<name> <sources...>) ────────────────────────────────
# Збирає ELF, підключає include ServoLib + OCM3, лінкує opencm3_stm32f4.
# Після збірки генерує .bin і виводить розміри секцій.

function(stm32_add_executable TARGET)
    add_executable(${TARGET} ${ARGN})

    target_include_directories(${TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/Inc
        ${CMAKE_SOURCE_DIR}/Board/${BOARD}
        ${OCM3_DIR}/include
    )

    target_link_directories(${TARGET} PRIVATE
        ${OCM3_DIR}/lib
    )

    # Порядок бібліотек важливий: opencm3 → m → c → gcc → nosys
    target_link_libraries(${TARGET} PRIVATE opencm3_stm32f4 m c gcc nosys)

    target_link_options(${TARGET} PRIVATE
        -T${LINKER_SCRIPT}
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
endfunction()
