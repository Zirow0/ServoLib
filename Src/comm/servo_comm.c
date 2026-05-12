#include "comm/servo_comm.h"
#include "comm/servo_protocol_desc.h"

#include "frame_codec.h"
#include "packet_codec.h"

#include <string.h>

/* ================================================================
 * Розміри буферів
 * max payload telemetry: sizeof(servo_telemetry_t) = 21
 * packet_raw = 1 + 21 = 22
 * frame_encoded = FRAME_ENCODED_SIZE(22) = (22+4) + 0 + 2 = 28
 *
 * max payload command: sizeof(servo_command_t) = 5
 * packet_raw = 6, frame_encoded = 12
 * ================================================================ */
#define COMM_RAW_BUF_SIZE    (1U + sizeof(servo_telemetry_t))
#define COMM_FRAME_BUF_SIZE  FRAME_ENCODED_SIZE(COMM_RAW_BUF_SIZE)

/* RX staging буфер — заповнюється в ISR, читається в main loop */
#define COMM_RX_STAGING_SIZE  64U

static volatile uint8_t rx_staging[COMM_RX_STAGING_SIZE];
static volatile size_t  rx_staging_len = 0;
static volatile int     rx_ready       = 0;

/* Остання декодована команда */
static servo_command_t  pending_cmd;
static volatile int     cmd_ready = 0;

/* Send callback */
static servo_comm_send_fn g_send_fn = NULL;

/* ================================================================
 * Публічні функції
 * ================================================================ */

void servo_comm_init(servo_comm_send_fn send_fn)
{
    g_send_fn     = send_fn;
    rx_ready      = 0;
    rx_staging_len = 0;
    cmd_ready     = 0;
}

/* ISR: тільки копіює байти */
void servo_comm_on_rx(const uint8_t *data, size_t len)
{
    if (rx_ready) return;  /* попередній кадр ще не оброблено */
    if (len > COMM_RX_STAGING_SIZE) return;

    memcpy((uint8_t *)rx_staging, data, len);
    rx_staging_len = len;
    rx_ready = 1;
}

/* Main loop: декодує та диспетчеризує */
void servo_comm_process_rx(void)
{
    if (!rx_ready) return;

    uint8_t raw[PACKET_RAW_SIZE(PACKET_MAX_PAYLOAD)];
    size_t  raw_len = frame_decode((const uint8_t *)rx_staging,
                                   rx_staging_len,
                                   raw, sizeof(raw));

    rx_ready = 0;  /* звільнити staging для наступного кадру */

    if (raw_len == 0) return;

    uint8_t msg_id;
    uint8_t payload[PACKET_MAX_PAYLOAD];
    size_t  plen = packet_decode(raw, raw_len,
                                 &msg_id, payload, sizeof(payload));

    if (plen == 0) return;
    if (MSG_TYPE(msg_id) != (uint8_t)MSG_TYPE_COMMAND) return;

    if (MSG_MODE(msg_id) == MSG_MODE_STRUCT) {
        if (plen == sizeof(servo_command_t)) {
            memcpy(&pending_cmd, payload, sizeof(servo_command_t));
            cmd_ready = 1;
        }
    } else if (MSG_MODE(msg_id) == MSG_MODE_PARAM) {
        proto_unpack_param(&pending_cmd, payload, plen);
        cmd_ready = 1;
    }
}

bool servo_comm_get_command(servo_command_t *cmd_out)
{
    if (!cmd_ready) return false;
    cmd_ready = 0;
    *cmd_out = pending_cmd;
    return true;
}

void servo_comm_send_telemetry(const servo_telemetry_t *telem)
{
    if (!g_send_fn) return;

    uint8_t raw[COMM_RAW_BUF_SIZE];
    uint8_t frame[COMM_FRAME_BUF_SIZE];

    size_t raw_len = packet_encode(MSG_TELEMETRY_STRUCT,
                                   (const uint8_t *)telem, sizeof(*telem),
                                   raw, sizeof(raw));
    if (raw_len == 0) return;

    size_t n = frame_encode(raw, raw_len, frame, sizeof(frame));
    if (n > 0) {
        g_send_fn(frame, n);
    }
}

void servo_comm_send_param(const servo_telemetry_t *telem, uint8_t field_idx)
{
    if (!g_send_fn || field_idx >= TELEM_FIELD_COUNT) return;

    const field_desc_t *fd = &telemetry_fields[field_idx];

    uint8_t payload[2U + 4U];  /* offset + type + max 4 байти значення */
    size_t  plen = proto_pack_param(telem, fd->offset, fd->type, payload);

    uint8_t raw[PACKET_RAW_SIZE(sizeof(payload))];
    uint8_t frame[FRAME_ENCODED_SIZE(PACKET_RAW_SIZE(sizeof(payload)))];

    size_t raw_len = packet_encode(MSG_TELEMETRY_PARAM,
                                   payload, plen,
                                   raw, sizeof(raw));
    if (raw_len == 0) return;

    size_t n = frame_encode(raw, raw_len, frame, sizeof(frame));
    if (n > 0) {
        g_send_fn(frame, n);
    }
}
