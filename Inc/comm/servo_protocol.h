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
    MSG_TYPE_TELEMETRY      = 0x01,  /* servo_telemetry_t   */
    MSG_TYPE_COMMAND        = 0x02,  /* servo_command_t     */
    MSG_TYPE_CASCADE_TELEM  = 0x03,  /* cascade_telemetry_t */
    MSG_TYPE_CASCADE_CONFIG = 0x04,  /* cascade_config_t    */
    MSG_TYPE_STOP           = 0x05,  /* без payload         */
    MSG_TYPE_ESTOP          = 0x06,  /* без payload, миттєво */
} servo_msg_type_t;

#define MSG_TELEMETRY_STRUCT      MSG_ID(MSG_TYPE_TELEMETRY,      MSG_MODE_STRUCT)
#define MSG_TELEMETRY_PARAM       MSG_ID(MSG_TYPE_TELEMETRY,      MSG_MODE_PARAM)
#define MSG_COMMAND_STRUCT        MSG_ID(MSG_TYPE_COMMAND,        MSG_MODE_STRUCT)
#define MSG_COMMAND_PARAM         MSG_ID(MSG_TYPE_COMMAND,        MSG_MODE_PARAM)
#define MSG_CASCADE_TELEM_STRUCT  MSG_ID(MSG_TYPE_CASCADE_TELEM,  MSG_MODE_STRUCT)
#define MSG_CASCADE_TELEM_PARAM   MSG_ID(MSG_TYPE_CASCADE_TELEM,  MSG_MODE_PARAM)
#define MSG_CASCADE_CFG_STRUCT    MSG_ID(MSG_TYPE_CASCADE_CONFIG, MSG_MODE_STRUCT)
#define MSG_CASCADE_CFG_PARAM     MSG_ID(MSG_TYPE_CASCADE_CONFIG, MSG_MODE_PARAM)
#define MSG_STOP_STRUCT           MSG_ID(MSG_TYPE_STOP,           MSG_MODE_STRUCT)
#define MSG_ESTOP_STRUCT          MSG_ID(MSG_TYPE_ESTOP,          MSG_MODE_STRUCT)

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
 * Каскадна телеметрія: STM32 → хост, 100 Hz
 * Повний знімок внутрішнього стану каскадного PID
 * ================================================================ */
typedef struct __attribute__((packed)) {
    /* Заголовок */
    uint32_t timestamp_ms;      /* 0   мс з запуску                         */

    /* Сенсори — вхід контролера */
    float    position_rad;      /* 4   θ,  рад                              */
    float    velocity_rad_s;    /* 8   ω,  рад/с                            */
    float    current_a;         /* 12  I,  А                                */

    /* Цілі та сигнальний ланцюг */
    float    target;            /* 16  активний setpoint (рад/рад·с/А)      */
    float    vel_sp;            /* 20  вихід pos-PID → вхід vel-PID, рад/с  */
    float    current_sp;        /* 24  вихід vel-PID → вхід trq-PID, А      */
    float    ff;                /* 28  внесок feedforward, %                */
    float    power;             /* 32  команда двигуну після slew, %        */

    /* pos-PID складові */
    float    pos_p;             /* 36  P-term, рад/с                        */
    float    pos_i;             /* 40  I-term, рад/с                        */
    float    pos_d;             /* 44  D-term, рад/с                        */

    /* vel-PID складові */
    float    vel_p;             /* 48  P-term, А                            */
    float    vel_i;             /* 52  I-term, А                            */
    float    vel_d;             /* 56  D-term, А                            */
    float    vel_integral;      /* 60  стан інтегратора (anti-windup), А    */

    /* trq-PID складові */
    float    trq_p;             /* 64  P-term, %                            */
    float    trq_i;             /* 68  I-term, %                            */
    float    trq_d;             /* 72  D-term, %                            */
    float    trq_integral;      /* 76  стан інтегратора (anti-windup), %    */

    /* Режим */
    uint8_t  mode;              /* 80  Cascade_Mode_t: 0=POS,1=VEL,2=TRQ   */
} cascade_telemetry_t;          /* 81 байт                                  */

/* ================================================================
 * Конфігурація каскадного PID: хост → STM32 (та STM32 → хост)
 * Відображає Cascade_Config_t у wire-форматі
 * ================================================================ */
typedef struct __attribute__((packed)) {
    /* pos-PID */
    float pos_kp;       /* 0   рад/с / рад              */
    float pos_ki;       /* 4                             */
    float pos_kd;       /* 8                             */
    float pos_out_min;  /* 12  рад/с                     */
    float pos_out_max;  /* 16  рад/с                     */
    float pos_i_limit;  /* 20  рад/с (0 = auto)          */

    /* vel-PID */
    float vel_kp;       /* 24  А / (рад/с)               */
    float vel_ki;       /* 28                             */
    float vel_kd;       /* 32                             */
    float vel_out_min;  /* 36  А (робочий ліміт струму)  */
    float vel_out_max;  /* 40  А                         */
    float vel_i_limit;  /* 44  А (0 = auto)              */

    /* trq-PID */
    float trq_kp;       /* 48  % / А                     */
    float trq_ki;       /* 52                             */
    float trq_kd;       /* 56                             */
    float trq_out_min;  /* 60  %                         */
    float trq_out_max;  /* 64  %                         */
    float trq_i_limit;  /* 68  % (0 = auto)              */

    /* Feedforward та slew */
    float ff_j;         /* 72  %/А   (FF інерції)        */
    float ff_b;         /* 76  %·с/рад (FF тертя)        */
    float slew_rate;    /* 80  %/с (0 = вимкнено)        */
} cascade_config_t;     /* 84 байт                       */

/* ================================================================
 * Індекси полів servo_telemetry_t для single-param режиму
 * ================================================================ */
#define TELEM_FIELD_POS       0   /* position_rad    */
#define TELEM_FIELD_VEL       1   /* velocity_rad_s  */
#define TELEM_FIELD_CUR       2   /* current_a       */
#define TELEM_FIELD_TARGET    3   /* target          */
#define TELEM_FIELD_MODE      4   /* mode            */
#define TELEM_FIELD_TS        5   /* timestamp_ms    */
#define TELEM_FIELD_COUNT     6

/* ================================================================
 * Індекси полів cascade_telemetry_t для single-param режиму
 * ================================================================ */
#define CASC_TELEM_TS           0   /* timestamp_ms    */
#define CASC_TELEM_POS          1   /* position_rad    */
#define CASC_TELEM_VEL          2   /* velocity_rad_s  */
#define CASC_TELEM_CUR          3   /* current_a       */
#define CASC_TELEM_TARGET       4   /* target          */
#define CASC_TELEM_VEL_SP       5   /* vel_sp          */
#define CASC_TELEM_CURRENT_SP   6   /* current_sp      */
#define CASC_TELEM_FF           7   /* ff              */
#define CASC_TELEM_POWER        8   /* power           */
#define CASC_TELEM_POS_P        9   /* pos_p           */
#define CASC_TELEM_POS_I       10   /* pos_i           */
#define CASC_TELEM_POS_D       11   /* pos_d           */
#define CASC_TELEM_VEL_P       12   /* vel_p           */
#define CASC_TELEM_VEL_I       13   /* vel_i           */
#define CASC_TELEM_VEL_D       14   /* vel_d           */
#define CASC_TELEM_VEL_INT     15   /* vel_integral    */
#define CASC_TELEM_TRQ_P       16   /* trq_p           */
#define CASC_TELEM_TRQ_I       17   /* trq_i           */
#define CASC_TELEM_TRQ_D       18   /* trq_d           */
#define CASC_TELEM_TRQ_INT     19   /* trq_integral    */
#define CASC_TELEM_MODE        20   /* mode            */
#define CASC_TELEM_FIELD_COUNT 21

/* ================================================================
 * Індекси полів cascade_config_t для single-param режиму
 * ================================================================ */
#define CASC_CFG_POS_KP        0
#define CASC_CFG_POS_KI        1
#define CASC_CFG_POS_KD        2
#define CASC_CFG_POS_OUT_MIN   3
#define CASC_CFG_POS_OUT_MAX   4
#define CASC_CFG_POS_I_LIMIT   5
#define CASC_CFG_VEL_KP        6
#define CASC_CFG_VEL_KI        7
#define CASC_CFG_VEL_KD        8
#define CASC_CFG_VEL_OUT_MIN   9
#define CASC_CFG_VEL_OUT_MAX  10
#define CASC_CFG_VEL_I_LIMIT  11
#define CASC_CFG_TRQ_KP       12
#define CASC_CFG_TRQ_KI       13
#define CASC_CFG_TRQ_KD       14
#define CASC_CFG_TRQ_OUT_MIN  15
#define CASC_CFG_TRQ_OUT_MAX  16
#define CASC_CFG_TRQ_I_LIMIT  17
#define CASC_CFG_FF_J         18
#define CASC_CFG_FF_B         19
#define CASC_CFG_SLEW_RATE    20
#define CASC_CFG_FIELD_COUNT  21

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
