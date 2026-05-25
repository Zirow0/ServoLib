#include "comm/servo_comm.h"
#include "comm/servo_protocol_desc.h"

#include "frame_codec.h"
#include "packet_codec.h"

#include <string.h>

/* ================================================================
 * Розміри буферів
 *
 * Найбільші структури:
 *   cascade_config_t    = 84 байт → packet_raw=85, frame=91
 *   cascade_telemetry_t = 81 байт → packet_raw=82, frame=88
 *   servo_telemetry_t   = 21 байт → packet_raw=22, frame=28
 *
 * COMM_MAX_PAYLOAD — найбільший payload (cascade_config_t)
 * COMM_TX_BUF_SIZE у board_config.h повинен бути ≥ COMM_FRAME_BUF_SIZE
 * ================================================================ */
#define COMM_MAX_PAYLOAD    sizeof(cascade_config_t)
#define COMM_RAW_BUF_SIZE   (1U + COMM_MAX_PAYLOAD)
#define COMM_FRAME_BUF_SIZE FRAME_ENCODED_SIZE(COMM_RAW_BUF_SIZE)

/* RX staging буфер — заповнюється в ISR, читається в main loop */
#define COMM_RX_STAGING_SIZE  96U

static volatile uint8_t rx_staging[COMM_RX_STAGING_SIZE];
static volatile size_t  rx_staging_len = 0;
static volatile int     rx_ready       = 0;

/* Pending повідомлення від хоста */
static servo_command_t  pending_cmd;
static volatile int     cmd_ready = 0;

static cascade_config_t pending_cascade_cfg;
static volatile int     cascade_cfg_ready = 0;

static volatile int     stop_ready = 0;

/* Send callback */
static servo_comm_send_fn g_send_fn = NULL;

/* ================================================================
 * Публічні функції
 * ================================================================ */

void servo_comm_init(servo_comm_send_fn send_fn)
{
    g_send_fn         = send_fn;
    rx_ready          = 0;
    rx_staging_len    = 0;
    cmd_ready         = 0;
    cascade_cfg_ready = 0;
    stop_ready        = 0;
    memset(&pending_cascade_cfg, 0, sizeof(pending_cascade_cfg));
}

void servo_comm_seed_cascade_config(const cascade_config_t *cfg)
{
    if (cfg == NULL) return;
    pending_cascade_cfg = *cfg;
}

/* ISR: тільки копіює байти.
 * Повертає true якщо дані прийняті, false якщо staging зайнятий — щоб
 * IDLE handler не просував rx_last_pos і наступний IDLE повторив доставку. */
bool servo_comm_on_rx(const uint8_t *data, size_t len)
{
    if (rx_ready) return false;
    if (len > COMM_RX_STAGING_SIZE) return false;

    memcpy((uint8_t *)rx_staging, data, len);
    rx_staging_len = len;
    rx_ready = 1;
    return true;
}

/* Main loop: декодує та диспетчеризує */
void servo_comm_process_rx(void)
{
    if (!rx_ready) return;

    uint8_t raw[PACKET_RAW_SIZE(PACKET_MAX_PAYLOAD)];
    size_t  raw_len = frame_decode((const uint8_t *)rx_staging,
                                   rx_staging_len,
                                   raw, sizeof(raw));

    rx_ready = 0;

    if (raw_len == 0) return;

    uint8_t msg_id;
    uint8_t payload[PACKET_MAX_PAYLOAD];
    size_t  plen = packet_decode(raw, raw_len,
                                 &msg_id, payload, sizeof(payload));

    if (plen == 0) return;

    uint8_t msg_type = MSG_TYPE(msg_id);
    uint8_t msg_mode = MSG_MODE(msg_id);

    if (msg_type == (uint8_t)MSG_TYPE_COMMAND) {
        if (msg_mode == MSG_MODE_STRUCT) {
            if (plen == sizeof(servo_command_t)) {
                memcpy(&pending_cmd, payload, sizeof(servo_command_t));
                cmd_ready = 1;
            }
        } else {
            proto_unpack_param(&pending_cmd, payload, plen);
            cmd_ready = 1;
        }
    } else if (msg_type == (uint8_t)MSG_TYPE_CASCADE_CONFIG) {
        if (msg_mode == MSG_MODE_STRUCT) {
            if (plen == sizeof(cascade_config_t)) {
                memcpy(&pending_cascade_cfg, payload, sizeof(cascade_config_t));
                cascade_cfg_ready = 1;
            }
        } else {
            proto_unpack_param(&pending_cascade_cfg, payload, plen);
            cascade_cfg_ready = 1;
        }
    } else if (msg_type == (uint8_t)MSG_TYPE_STOP) {
        stop_ready = 1;
    }
}

bool servo_comm_get_command(servo_command_t *cmd_out)
{
    if (!cmd_ready) return false;
    cmd_ready = 0;
    *cmd_out = pending_cmd;
    return true;
}

bool servo_comm_get_stop(void)
{
    if (!stop_ready) return false;
    stop_ready = 0;
    return true;
}

bool servo_comm_get_cascade_config(cascade_config_t *cfg_out)
{
    if (!cascade_cfg_ready) return false;
    cascade_cfg_ready = 0;
    *cfg_out = pending_cascade_cfg;
    return true;
}

/* ================================================================
 * Внутрішній хелпер: encode → send
 * ================================================================ */
static void comm_send_frame(uint8_t msg_id,
                            const uint8_t *payload, size_t payload_len)
{
    if (!g_send_fn) return;

    uint8_t raw[COMM_RAW_BUF_SIZE];
    uint8_t frame[COMM_FRAME_BUF_SIZE];

    size_t raw_len = packet_encode(msg_id, payload, payload_len,
                                   raw, sizeof(raw));
    if (raw_len == 0) return;

    size_t n = frame_encode(raw, raw_len, frame, sizeof(frame));
    if (n > 0) {
        g_send_fn(frame, n);
    }
}

/* ================================================================
 * TX функції
 * ================================================================ */

void servo_comm_send_telemetry(const servo_telemetry_t *telem)
{
    comm_send_frame(MSG_TELEMETRY_STRUCT,
                    (const uint8_t *)telem, sizeof(*telem));
}

void servo_comm_send_param(const servo_telemetry_t *telem, uint8_t field_idx)
{
    if (field_idx >= TELEM_FIELD_COUNT) return;
    const field_desc_t *fd = &telemetry_fields[field_idx];
    uint8_t payload[2U + 4U];
    size_t  plen = proto_pack_param(telem, fd->offset, fd->type, payload);
    comm_send_frame(MSG_TELEMETRY_PARAM, payload, plen);
}

void servo_comm_send_cascade(const cascade_telemetry_t *telem)
{
    comm_send_frame(MSG_CASCADE_TELEM_STRUCT,
                    (const uint8_t *)telem, sizeof(*telem));
}

void servo_comm_send_cascade_param(const cascade_telemetry_t *telem,
                                   uint8_t field_idx)
{
    if (field_idx >= CASC_TELEM_FIELD_COUNT) return;
    const field_desc_t *fd = &cascade_telem_fields[field_idx];
    uint8_t payload[2U + 4U];
    size_t  plen = proto_pack_param(telem, fd->offset, fd->type, payload);
    comm_send_frame(MSG_CASCADE_TELEM_PARAM, payload, plen);
}

void servo_comm_send_cascade_config(const cascade_config_t *cfg)
{
    comm_send_frame(MSG_CASCADE_CFG_STRUCT,
                    (const uint8_t *)cfg, sizeof(*cfg));
}
