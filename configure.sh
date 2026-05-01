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
echo "Оберіть тип програматора (або 'пропустити' — питати при кожній прошивці):"
PS3="Введіть номер: "
select choice in "stlink" "daplink" "jlink" "пропустити"; do
    [[ -n "${choice}" ]] && break
    echo "Невірний вибір."
done

PROGRAMMER=""
PROGRAMMER_SERIAL=""

if [[ "${choice}" != "пропустити" ]]; then
    PROGRAMMER="${choice}"

    # Визначаємо шаблон пошуку для цього типу
    case "${PROGRAMMER}" in
        stlink)  USB_PATTERN="st-link|stlink"    ;;
        daplink) USB_PATTERN="cmsis-dap|daplink" ;;
        jlink)   USB_PATTERN="j-link"            ;;
    esac

    # Шукаємо підключені пристрої через sysfs
    prog_serials=()
    prog_names=()
    for dir in /sys/bus/usb/devices/*/; do
        product=$(cat "${dir}product" 2>/dev/null || true)
        if echo "${product}" | grep -qiE "${USB_PATTERN}"; then
            serial=$(cat "${dir}serial" 2>/dev/null || true)
            if [[ -n "${serial}" ]]; then
                prog_serials+=("${serial}")
                prog_names+=("${product}")
            fi
        fi
    done

    case ${#prog_serials[@]} in
        0)
            echo "Попередження: ${PROGRAMMER} не знайдено. ID буде запитано при прошивці."
            ;;
        1)
            PROGRAMMER_SERIAL="${prog_serials[0]}"
            echo "→ Знайдено: ${prog_names[0]}  (ID: ${PROGRAMMER_SERIAL})"
            ;;
        *)
            echo "Знайдено кілька пристроїв ${PROGRAMMER}:"
            PS3="Оберіть пристрій: "
            display_items=()
            for i in "${!prog_serials[@]}"; do
                display_items+=("${prog_names[$i]}  (ID: ${prog_serials[$i]})")
            done
            select item in "${display_items[@]}"; do
                idx=$((REPLY - 1))
                if [[ "${idx}" -ge 0 && "${idx}" -lt ${#prog_serials[@]} ]]; then
                    PROGRAMMER_SERIAL="${prog_serials[${idx}]}"
                    break
                fi
                echo "Невірний вибір."
            done
            ;;
    esac
fi

# ─── CMake конфігурація ───────────────────────────────────────────────────────
BUILD_DIR="${SCRIPT_DIR}/build/${BOARD}/${APP}"

echo ""
echo "→ Плата:      ${BOARD}"
echo "→ Ціль:       ${APP}"
if [[ -n "${PROGRAMMER}" ]]; then
    echo "→ Програматор: ${PROGRAMMER}${PROGRAMMER_SERIAL:+  (ID: ${PROGRAMMER_SERIAL})}"
else
    echo "→ Програматор: <питати при прошивці>"
fi
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
PROGRAMMER_SERIAL="${PROGRAMMER_SERIAL}"
EOF

echo ""
echo "Конфігурацію завершено. Далі:"
echo "  ./build.sh"
echo "  ./flash.sh"
