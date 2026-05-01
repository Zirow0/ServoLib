#!/usr/bin/env bash
# flash.sh — прошивка плати через OpenOCD
#
# Виклик із cmake target (flash.sh <hex> <openocd_target_cfg>):
#   аргументи передані явно, PROGRAMMER береться з .preset або питається
#
# Виклик вручну (flash.sh):
#   усе визначається з .preset
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRESET_FILE="${SCRIPT_DIR}/.preset"

# ─── Читання .preset ──────────────────────────────────────────────────────────
if [[ ! -f "${PRESET_FILE}" ]]; then
    echo "Помилка: проект не сконфігуровано."
    echo "Запустіть спочатку: ./configure.sh"
    exit 1
fi

# shellcheck source=/dev/null
source "${PRESET_FILE}"

# ─── Аргументи або авто-визначення шляхів ────────────────────────────────────
if [[ $# -eq 2 ]]; then
    # Виклик із cmake --build --target flash
    HEX_FILE="$1"
    OPENOCD_TARGET_CFG="$2"
else
    # Самостійний виклик — визначаємо шляхи з .preset
    BUILD_DIR="${SCRIPT_DIR}/build/${BOARD}/${APP}"
    HEX_FILE="${BUILD_DIR}/Apps/${APP}/${APP}.hex"
    # Для самостійного виклику збираємо спочатку
    echo "→ Збірка..."
    cmake --build "${BUILD_DIR}"
fi

if [[ ! -f "${HEX_FILE}" ]]; then
    echo "Помилка: прошивку не знайдено: ${HEX_FILE}"
    echo "Запустіть спочатку: ./build.sh"
    exit 1
fi

# ─── Визначення OPENOCD_TARGET_CFG якщо не передано ──────────────────────────
if [[ -z "${OPENOCD_TARGET_CFG:-}" ]]; then
    # Читаємо з CMakeCache якщо доступний
    CACHE="${SCRIPT_DIR}/build/${BOARD}/${APP}/CMakeCache.txt"
    if [[ -f "${CACHE}" ]]; then
        family=$(grep -m1 "^GENLINK_FAMILY" "${CACHE}" 2>/dev/null | cut -d= -f2 || true)
        [[ -n "${family}" ]] && OPENOCD_TARGET_CFG="target/${family}x.cfg"
    fi
    # Fallback
    OPENOCD_TARGET_CFG="${OPENOCD_TARGET_CFG:-target/stm32f4x.cfg}"
fi

# ─── Вибір програматора ───────────────────────────────────────────────────────
if [[ -z "${PROGRAMMER:-}" ]]; then
    echo "Оберіть програматор:"
    PS3="Введіть номер: "
    select choice in "stlink" "daplink" "jlink"; do
        [[ -n "${choice}" ]] && PROGRAMMER="${choice}" && break
        echo "Невірний вибір."
    done
fi

case "${PROGRAMMER}" in
    stlink)  IFACE_CFG="interface/stlink.cfg"    USB_PATTERN="st-link|stlink"    SERIAL_CMD="hla_serial"       ;;
    daplink) IFACE_CFG="interface/cmsis-dap.cfg" USB_PATTERN="cmsis-dap|daplink" SERIAL_CMD="cmsis_dap_serial" ;;
    jlink)   IFACE_CFG="interface/jlink.cfg"     USB_PATTERN="j-link"            SERIAL_CMD="jlink_serial"     ;;
    *) echo "Невідомий програматор: ${PROGRAMMER}"; exit 1 ;;
esac

# ─── Вибір конкретного пристрою ───────────────────────────────────────────────
# Якщо збережено конкретний ID — використовуємо одразу
if [[ -n "${PROGRAMMER_SERIAL:-}" ]]; then
    SERIAL="${PROGRAMMER_SERIAL}"
    echo "→ Програматор: ${PROGRAMMER}  (ID: ${SERIAL})"
else
    # Пошук підключених пристроїв через sysfs
    serials=()
    names=()

    for dir in /sys/bus/usb/devices/*/; do
        product=$(cat "${dir}product" 2>/dev/null || true)
        if echo "${product}" | grep -qiE "${USB_PATTERN}"; then
            serial=$(cat "${dir}serial" 2>/dev/null || true)
            if [[ -n "${serial}" ]]; then
                serials+=("${serial}")
                names+=("${product}")
            fi
        fi
    done

    case ${#serials[@]} in
        0)
            echo "Помилка: ${PROGRAMMER} не знайдено."
            exit 1
            ;;
        1)
            SERIAL="${serials[0]}"
            echo "→ Знайдено: ${names[0]}  (ID: ${SERIAL})"
            ;;
        *)
            echo "Знайдено кілька пристроїв ${PROGRAMMER}:"
            PS3="Оберіть пристрій: "
            display_items=()
            for i in "${!serials[@]}"; do
                display_items+=("${names[$i]}  (ID: ${serials[$i]})")
            done
            select item in "${display_items[@]}"; do
                idx=$((REPLY - 1))
                if [[ "${idx}" -ge 0 && "${idx}" -lt ${#serials[@]} ]]; then
                    SERIAL="${serials[${idx}]}"
                    break
                fi
                echo "Невірний вибір."
            done
            ;;
    esac
fi

# ─── Прошивка ─────────────────────────────────────────────────────────────────
echo "→ Target cfg:  ${OPENOCD_TARGET_CFG}"
echo "→ HEX:         ${HEX_FILE}"
echo ""

openocd \
    -f "${IFACE_CFG}" \
    -c "${SERIAL_CMD} ${SERIAL}" \
    -f "${OPENOCD_TARGET_CFG}" \
    -c "program ${HEX_FILE} verify reset exit"
