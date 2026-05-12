#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ================================================================
 * msg_id: біти [7:1] = тип повідомлення, біт[0] = режим
 *   0 = повна структура (struct mode)
 *   1 = один параметр  (param mode): [offset][type][value]
 * ================================================================ */
#define MSG_ID(type, mode)  (uint8_t)(((uint8_t)(type) << 1) | ((mode) & 0x01))
#define MSG_TYPE(id)        ((id) >> 1)
#define MSG_MODE(id)        ((id) & 0x01)

#define MSG_MODE_STRUCT  0
#define MSG_MODE_PARAM   1

/* ================================================================
 * Типи повідомлень (7 біт → максимум 127 типів)
 * ================================================================ */
typedef enum {
    MSG_TYPE_TELEMETRY = 0x01,  /* servo_telemetry_t */
    MSG_TYPE_COMMAND   = 0x02,  /* servo_command_t   */
} servo_msg_type_t;

#define MSG_TELEMETRY_STRUCT  MSG_ID(MSG_TYPE_TELEMETRY, MSG_MODE_STRUCT)
#define MSG_TELEMETRY_PARAM   MSG_ID(MSG_TYPE_TELEMETRY, MSG_MODE_PARAM)
#define MSG_COMMAND_STRUCT    MSG_ID(MSG_TYPE_COMMAND,   MSG_MODE_STRUCT)
#define MSG_COMMAND_PARAM     MSG_ID(MSG_TYPE_COMMAND,   MSG_MODE_PARAM)

/* ================================================================
 * Типи параметрів
 * ================================================================ */
typedef enum {
    PTYPE_INT8   = 0x01,
    PTYPE_UINT8  = 0x02,
    PTYPE_INT16  = 0x03,
    PTYPE_UINT16 = 0x04,
    PTYPE_INT32  = 0x05,
    PTYPE_UINT32 = 0x06,
    PTYPE_FLOAT  = 0x07,
} param_type_t;

static inline uint8_t param_size(param_type_t t)
{
    static const uint8_t sz[] = { 0, 1, 1, 2, 2, 4, 4, 4 };
    return sz[(uint8_t)t < 8u ? t : 0u];
}

/* ================================================================
 * Дескриптор поля структури (2 байти у flash)
 * ================================================================ */
typedef struct {
    uint8_t      offset;
    param_type_t type;
} field_desc_t;

#define FIELD(S, F, T)  { .offset = (uint8_t)offsetof(S, F), .type = (T) }

/* ================================================================
 * Структури даних протоколу (packed — без паддінгу для UART)
 * ================================================================ */

/* Телеметрія: STM32 → хост, 100 Hz */
typedef struct __attribute__((packed)) {
    float    position_rad;     /* поточне положення, рад          */
    float    velocity_rad_s;   /* поточна швидкість, рад/с        */
    float    current_a;        /* поточний струм, А               */
    float    target;           /* ціль (рад / рад·с / А per mode) */
    uint8_t  mode;             /* Cascade_Mode_t                  */
    uint32_t timestamp_ms;     /* мілісекунди з запуску           */
} servo_telemetry_t;           /* 4+4+4+4+1+4 = 21 байт           */

/* Команда: хост → STM32 */
typedef struct __attribute__((packed)) {
    uint8_t  mode;    /* Cascade_Mode_t: 0=POS, 1=VEL, 2=TRQ */
    float    target;  /* рад / рад·с / А залежно від mode    */
} servo_command_t;    /* 1+4 = 5 байт                         */

/* ================================================================
 * Індекси полів телеметрії для single-param режиму
 * ================================================================ */
#define TELEM_FIELD_POS       0   /* position_rad    */
#define TELEM_FIELD_VEL       1   /* velocity_rad_s  */
#define TELEM_FIELD_CUR       2   /* current_a       */
#define TELEM_FIELD_TARGET    3   /* target          */
#define TELEM_FIELD_MODE      4   /* mode            */
#define TELEM_FIELD_TS        5   /* timestamp_ms    */
#define TELEM_FIELD_COUNT     6

/* ================================================================
 * Пакування / розпакування одного параметра
 *
 * proto_pack_param: [offset 1B][type 1B][value N bytes] → out
 * proto_unpack_param: payload → dst_struct
 * ================================================================ */
static inline size_t proto_pack_param(const void   *src_struct,
                                      uint8_t       offset,
                                      param_type_t  type,
                                      uint8_t      *out)
{
    uint8_t sz = param_size(type);
    out[0] = offset;
    out[1] = (uint8_t)type;
    memcpy(&out[2], (const uint8_t *)src_struct + offset, sz);
    return 2u + sz;
}

static inline int proto_unpack_param(void          *dst_struct,
                                     const uint8_t *payload,
                                     size_t         len)
{
    if (len < 3u) return 0;
    uint8_t      offset = payload[0];
    param_type_t type   = (param_type_t)payload[1];
    uint8_t      sz     = param_size(type);
    if (sz == 0u || len < 2u + sz) return 0;
    memcpy((uint8_t *)dst_struct + offset, &payload[2], sz);
    return 1;
}
