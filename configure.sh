#!/usr/bin/env bash
# configure.sh — вибір цілі збірки, конфігурація CMake, оновлення compile_commands.json і .clangd
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ─── Перевірка LIBOPENCM3_DIR ─────────────────────────────────────────────────
if [ -z "$LIBOPENCM3_DIR" ]; then
    echo "Помилка: LIBOPENCM3_DIR не встановлено."
    echo "Приклад: export LIBOPENCM3_DIR=/path/to/libopencm3"
    exit 1
fi

# ─── Меню вибору цілі ─────────────────────────────────────────────────────────
TARGETS=(debug_encoder debug_motor debug_brake servo_full)

echo "Оберіть ціль збірки:"
for i in "${!TARGETS[@]}"; do
    echo "  $((i+1))) ${TARGETS[$i]}"
done
echo ""

if [ -n "$1" ]; then
    PRESET="$1"
    valid=0
    for t in "${TARGETS[@]}"; do
        [ "$t" = "$PRESET" ] && valid=1 && break
    done
    if [ $valid -eq 0 ]; then
        echo "Невідома ціль: $PRESET"
        echo "Доступні: ${TARGETS[*]}"
        exit 1
    fi
else
    read -rp "Введіть номер [1-${#TARGETS[@]}]: " choice
    if ! [[ "$choice" =~ ^[1-9][0-9]*$ ]] || [ "$choice" -lt 1 ] || [ "$choice" -gt "${#TARGETS[@]}" ]; then
        echo "Невірний вибір."
        exit 1
    fi
    PRESET="${TARGETS[$((choice-1))]}"
fi

echo ""
echo "→ Ціль: $PRESET"

# ─── Конфігурація CMake ───────────────────────────────────────────────────────
echo "→ cmake --preset $PRESET -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
cmake --preset "$PRESET" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# ─── Симлінк compile_commands.json → корінь ───────────────────────────────────
COMPILE_COMMANDS="$SCRIPT_DIR/build/$PRESET/compile_commands.json"
if [ -f "$COMPILE_COMMANDS" ]; then
    ln -sf "$COMPILE_COMMANDS" "$SCRIPT_DIR/compile_commands.json"
    echo "→ compile_commands.json → build/$PRESET/compile_commands.json"
else
    echo "Попередження: $COMPILE_COMMANDS не знайдено."
fi

echo ""
echo "Конфігурацію завершено. Для збірки:"
echo "  cmake --build build/$PRESET"
