#!/usr/bin/env bash
# build.sh — збірка сконфігурованого проекту
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

echo "→ Ціль: $TARGET"
echo "→ cmake --build $BUILD_DIR"
cmake --build "$BUILD_DIR"
