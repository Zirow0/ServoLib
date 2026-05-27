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

/* cascade_telemetry_t — повний знімок каскадного PID */
static const field_desc_t cascade_telem_fields[CASC_TELEM_FIELD_COUNT] = {
    FIELD(cascade_telemetry_t, timestamp_ms,   PTYPE_UINT32),  /* [0]  */
    FIELD(cascade_telemetry_t, position_rad,   PTYPE_FLOAT),   /* [1]  */
    FIELD(cascade_telemetry_t, velocity_rad_s, PTYPE_FLOAT),   /* [2]  */
    FIELD(cascade_telemetry_t, accel_rad_s2,   PTYPE_FLOAT),   /* [3]  */
    FIELD(cascade_telemetry_t, current_a,      PTYPE_FLOAT),   /* [4]  */
    FIELD(cascade_telemetry_t, target,         PTYPE_FLOAT),   /* [5]  */
    FIELD(cascade_telemetry_t, vel_sp,         PTYPE_FLOAT),   /* [6]  */
    FIELD(cascade_telemetry_t, current_sp,     PTYPE_FLOAT),   /* [7]  */
    FIELD(cascade_telemetry_t, ff,             PTYPE_FLOAT),   /* [8]  */
    FIELD(cascade_telemetry_t, power,          PTYPE_FLOAT),   /* [9]  */
    FIELD(cascade_telemetry_t, pos_p,          PTYPE_FLOAT),   /* [10] */
    FIELD(cascade_telemetry_t, pos_i,          PTYPE_FLOAT),   /* [11] */
    FIELD(cascade_telemetry_t, pos_d,          PTYPE_FLOAT),   /* [12] */
    FIELD(cascade_telemetry_t, vel_p,          PTYPE_FLOAT),   /* [13] */
    FIELD(cascade_telemetry_t, vel_i,          PTYPE_FLOAT),   /* [14] */
    FIELD(cascade_telemetry_t, vel_d,          PTYPE_FLOAT),   /* [15] */
    FIELD(cascade_telemetry_t, vel_integral,   PTYPE_FLOAT),   /* [16] */
    FIELD(cascade_telemetry_t, trq_p,          PTYPE_FLOAT),   /* [17] */
    FIELD(cascade_telemetry_t, trq_i,          PTYPE_FLOAT),   /* [18] */
    FIELD(cascade_telemetry_t, trq_d,          PTYPE_FLOAT),   /* [19] */
    FIELD(cascade_telemetry_t, trq_integral,   PTYPE_FLOAT),   /* [20] */
    FIELD(cascade_telemetry_t, mode,           PTYPE_UINT8),   /* [21] */
};

/* cascade_config_t — параметри каскадного PID */
static const field_desc_t cascade_config_fields[CASC_CFG_FIELD_COUNT] = {
    FIELD(cascade_config_t, pos_kp,      PTYPE_FLOAT),  /* [0]  */
    FIELD(cascade_config_t, pos_ki,      PTYPE_FLOAT),  /* [1]  */
    FIELD(cascade_config_t, pos_kd,      PTYPE_FLOAT),  /* [2]  */
    FIELD(cascade_config_t, pos_out_min, PTYPE_FLOAT),  /* [3]  */
    FIELD(cascade_config_t, pos_out_max, PTYPE_FLOAT),  /* [4]  */
    FIELD(cascade_config_t, pos_i_limit, PTYPE_FLOAT),  /* [5]  */
    FIELD(cascade_config_t, vel_kp,      PTYPE_FLOAT),  /* [6]  */
    FIELD(cascade_config_t, vel_ki,      PTYPE_FLOAT),  /* [7]  */
    FIELD(cascade_config_t, vel_kd,      PTYPE_FLOAT),  /* [8]  */
    FIELD(cascade_config_t, vel_out_min, PTYPE_FLOAT),  /* [9]  */
    FIELD(cascade_config_t, vel_out_max, PTYPE_FLOAT),  /* [10] */
    FIELD(cascade_config_t, vel_i_limit, PTYPE_FLOAT),  /* [11] */
    FIELD(cascade_config_t, trq_kp,      PTYPE_FLOAT),  /* [12] */
    FIELD(cascade_config_t, trq_ki,      PTYPE_FLOAT),  /* [13] */
    FIELD(cascade_config_t, trq_kd,      PTYPE_FLOAT),  /* [14] */
    FIELD(cascade_config_t, trq_out_min, PTYPE_FLOAT),  /* [15] */
    FIELD(cascade_config_t, trq_out_max, PTYPE_FLOAT),  /* [16] */
    FIELD(cascade_config_t, trq_i_limit, PTYPE_FLOAT),  /* [17] */
    FIELD(cascade_config_t, ff_j,        PTYPE_FLOAT),  /* [18] */
    FIELD(cascade_config_t, ff_b,        PTYPE_FLOAT),  /* [19] */
    FIELD(cascade_config_t, slew_rate,   PTYPE_FLOAT),  /* [20] */
};
