#!/usr/bin/env bash
# configure.sh — інтерактивний вибір плати, цілі та програматора
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ─── Перевірка LIBOPENCM3_DIR ─────────────────────────────────────────────────
if [[ -z "${LIBOPENCM3_DIR:-}" ]]; then
    echo "Помилка: LIBOPENCM3_DIR не встановлено."
    echo "Приклад: export LIBOPENCM3_DIR=/path/to/libopencm3"
    exit 1
fi

# ─── Вибір плати ──────────────────────────────────────────────────────────────
mapfile -t boards < <(basename -s .cmake "${SCRIPT_DIR}"/cmake/targets/*.cmake | sort)

if [[ ${#boards[@]} -eq 0 ]]; then
    echo "Помилка: не знайдено жодного файлу в cmake/targets/"
    exit 1
elif [[ ${#boards[@]} -eq 1 ]]; then
    BOARD="${boards[0]}"
    echo "Плата: ${BOARD}"
else
    echo "Оберіть плату:"
    PS3="Введіть номер: "
    select BOARD in "${boards[@]}"; do
        [[ -n "${BOARD}" ]] && break
        echo "Невірний вибір."
    done
fi

# ─── Вибір застосунку ─────────────────────────────────────────────────────────
mapfile -t apps < <(find "${SCRIPT_DIR}/Apps" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort)

if [[ ${#apps[@]} -eq 0 ]]; then
    echo "Помилка: не знайдено жодного застосунку в Apps/"
    exit 1
fi

echo ""
echo "Оберіть ціль:"
PS3="Введіть номер: "
select APP in "${apps[@]}"; do
    [[ -n "${APP}" ]] && break
    echo "Невірний вибір."
done

# ─── Вибір програматора ───────────────────────────────────────────────────────
echo ""
echo "Оберіть програматор (або 'пропустити' — питати при кожній прошивці):"
PS3="Введіть номер: "
select choice in "stlink" "daplink" "jlink" "пропустити"; do
    [[ -n "${choice}" ]] && break
    echo "Невірний вибір."
done

if [[ "${choice}" == "пропустити" ]]; then
    PROGRAMMER=""
else
    PROGRAMMER="${choice}"
fi

# ─── CMake конфігурація ───────────────────────────────────────────────────────
BUILD_DIR="${SCRIPT_DIR}/build/${BOARD}/${APP}"

echo ""
echo "→ Плата:      ${BOARD}"
echo "→ Ціль:       ${APP}"
echo "→ Програматор: ${PROGRAMMER:-<питати при прошивці>}"
echo "→ Директорія: ${BUILD_DIR}"
echo ""

cmake -B "${BUILD_DIR}" \
      -G Ninja \
      -DBOARD="${BOARD}" \
      -DAPP="${APP}" \
      -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/toolchain/arm-none-eabi.cmake"

# ─── Збереження стану ─────────────────────────────────────────────────────────
cat > "${SCRIPT_DIR}/.preset" <<EOF
BOARD="${BOARD}"
APP="${APP}"
PROGRAMMER="${PROGRAMMER}"
EOF

echo ""
echo "Конфігурацію завершено. Далі:"
echo "  ./build.sh"
echo "  ./flash.sh"
