#pragma once

#ifdef USE_COMM_ASYNC

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Розміри буферів — задаються у board_config.h */
#ifndef COMM_RX_BUF_SIZE
#define COMM_RX_BUF_SIZE   64
#endif
#ifndef COMM_TX_BUF_SIZE
#define COMM_TX_BUF_SIZE   64
#endif
#ifndef COMM_TX_QUEUE_LEN
#define COMM_TX_QUEUE_LEN   4
#endif

typedef bool (*uart_rx_cb_t)(const uint8_t *data, size_t len);

/* Апаратна конфігурація — задається один раз у main.
 * RCC, NVIC, GPIO AF визначаються автоматично. */
typedef struct {
    uint32_t usart;

    uint32_t tx_port;  uint16_t tx_pin;
    uint32_t rx_port;  uint16_t rx_pin;

    uint32_t rx_dma;  uint8_t rx_stream;  uint8_t rx_channel;
    uint32_t tx_dma;  uint8_t tx_stream;  uint8_t tx_channel;
} uart_hw_config_t;

/* Внутрішній стан — не змінювати напряму */
typedef struct {
    const uart_hw_config_t *hw;
    uart_rx_cb_t            rx_cb;

    uint8_t  rx_buf[COMM_RX_BUF_SIZE];
    size_t   rx_last_pos;

    uint8_t  tx_pool[COMM_TX_QUEUE_LEN][COMM_TX_BUF_SIZE];
    uint8_t  tx_len[COMM_TX_QUEUE_LEN];
    uint8_t  tx_head;
    uint8_t  tx_tail;
    uint8_t  tx_count;
    volatile bool tx_busy;
} uart_instance_t;

void uart_init(uart_instance_t *inst,
               const uart_hw_config_t *hw,
               uint32_t baud,
               uart_rx_cb_t cb);

bool uart_send(uart_instance_t *inst, const uint8_t *data, size_t len);

/* Ініціалізація апаратного CRC32 блоку (RCC_CRC).
 * Викликати один раз до першого frame_encode/decode. */
void frame_crc32_init(void);

#endif /* USE_COMM_ASYNC */
