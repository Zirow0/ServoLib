#pragma once

/* Приватний файл — підключається тільки з servo_comm.c.
 * Таблиця полів для single-param режиму telemetry. */

#include "comm/servo_protocol.h"

#include <stddef.h>

static const field_desc_t telemetry_fields[TELEM_FIELD_COUNT] = {
    FIELD(servo_telemetry_t, position_rad,   PTYPE_FLOAT),    /* [0] */
    FIELD(servo_telemetry_t, velocity_rad_s, PTYPE_FLOAT),    /* [1] */
    FIELD(servo_telemetry_t, current_a,      PTYPE_FLOAT),    /* [2] */
    FIELD(servo_telemetry_t, target,         PTYPE_FLOAT),    /* [3] */
    FIELD(servo_telemetry_t, mode,           PTYPE_UINT8),    /* [4] */
    FIELD(servo_telemetry_t, timestamp_ms,   PTYPE_UINT32),   /* [5] */
};
