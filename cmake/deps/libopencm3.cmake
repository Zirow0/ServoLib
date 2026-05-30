# ─── libopencm3: пошук за пріоритетом ────────────────────────────────────────
# 1. Lib/libopencm3/   — git submodule або symlink (пріоритет, офлайн)
# 2. ENV{LIBOPENCM3_DIR} — змінна середовища (зворотна сумісність)

if(EXISTS "${CMAKE_SOURCE_DIR}/Lib/libopencm3/ld/devices.data")
    set(OCM3_DIR "${CMAKE_SOURCE_DIR}/Lib/libopencm3")
    message(STATUS "[libopencm3] Lib/libopencm3")
elseif(DEFINED ENV{LIBOPENCM3_DIR})
    set(OCM3_DIR "$ENV{LIBOPENCM3_DIR}")
    message(STATUS "[libopencm3] ENV → $ENV{LIBOPENCM3_DIR}")
else()
    message(FATAL_ERROR
        "libopencm3 не знайдено.\n"
        "Варіант 1 — git submodule (рекомендовано):\n"
        "  git submodule add https://github.com/libopencm3/libopencm3 Lib/libopencm3\n"
        "  git submodule update --init\n"
        "  make -C Lib/libopencm3\n"
        "Варіант 2 — змінна середовища:\n"
        "  export LIBOPENCM3_DIR=/path/to/libopencm3\n")
endif()
