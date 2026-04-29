#!/usr/bin/env bash
# build.sh — збірка сконфігурованого проекту
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRESET_FILE="${SCRIPT_DIR}/.preset"

if [[ ! -f "${PRESET_FILE}" ]]; then
    echo "Помилка: проект не сконфігуровано."
    echo "Запустіть спочатку: ./configure.sh"
    exit 1
fi

# shellcheck source=/dev/null
source "${PRESET_FILE}"

BUILD_DIR="${SCRIPT_DIR}/build/${BOARD}/${APP}"

echo "→ Плата: ${BOARD}  Ціль: ${APP}"
cmake --build "${BUILD_DIR}"
