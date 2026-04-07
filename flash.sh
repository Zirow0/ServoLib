#!/usr/bin/env bash
# flash.sh — завантаження прошивки на плату через OpenOCD
# Використання: ./flash.sh [stlink|daplink]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ─── Визначення поточної цілі ─────────────────────────────────────────────────
LINK=$(readlink "$SCRIPT_DIR/compile_commands.json" 2>/dev/null || true)
if [ -z "$LINK" ]; then
    echo "Помилка: проект не сконфігуровано."
    echo "Запусти спочатку: ./configure.sh"
    exit 1
fi

TARGET=$(basename "$(dirname "$LINK")")
BUILD_DIR="$SCRIPT_DIR/build/$TARGET"
HEX="$BUILD_DIR/Apps/$TARGET/$TARGET.hex"

if [ ! -f "$HEX" ]; then
    echo "Помилка: прошивку не знайдено: $HEX"
    echo "Запусти спочатку: ./build.sh"
    exit 1
fi

# ─── Визначення сімейства МК за платою ───────────────────────────────────────
BOARD=$(grep -m1 "^BOARD:" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)

case "$BOARD" in
    STM32F411*|STM32F4*)  MCU_CFG="target/stm32f4x.cfg" ;;
    STM32F1*)             MCU_CFG="target/stm32f1x.cfg" ;;
    STM32H7*)             MCU_CFG="target/stm32h7x.cfg" ;;
    *)                    MCU_CFG="target/stm32f4x.cfg" ;;
esac

# ─── Вибір програматора ───────────────────────────────────────────────────────
PROGRAMMER="${1:-}"

if [ -z "$PROGRAMMER" ]; then
    echo "Оберіть програматор:"
    echo "  1) stlink"
    echo "  2) daplink"
    read -rp "Введіть номер [1-2]: " choice
    case "$choice" in
        1) PROGRAMMER="stlink"  ;;
        2) PROGRAMMER="daplink" ;;
        *) echo "Невірний вибір."; exit 1 ;;
    esac
fi

case "$PROGRAMMER" in
    stlink)  IFACE_CFG="interface/stlink.cfg"     ;;
    daplink) IFACE_CFG="interface/cmsis-dap.cfg"  ;;
    *) echo "Невідомий програматор: $PROGRAMMER (stlink або daplink)"; exit 1 ;;
esac

# ─── Завантаження ─────────────────────────────────────────────────────────────
echo "→ Ціль:       $TARGET"
echo "→ Плата:      ${BOARD:-невідома} → $MCU_CFG"
echo "→ Програматор: $PROGRAMMER → $IFACE_CFG"
echo "→ HEX:        $HEX"
echo ""

openocd \
    -f "$IFACE_CFG" \
    -f "$MCU_CFG" \
    -c "program $HEX verify reset exit"
