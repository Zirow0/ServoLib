/*
 * Апаратна реалізація frame_crc32 для STM32F4.
 *
 * Перевизначає weak-символ з Lib/frame_codec/src/crc32_soft.c.
 * Алгоритм: CRC-32/MPEG-2 (поліном 0x04C11DB7, big-endian).
 *
 * УВАГА: апаратний блок CRC є єдиним глобальним ресурсом.
 * Не викликати frame_crc32 одночасно з ISR та main loop.
 * У servo_basic це забезпечується: ISR тільки копіює байти,
 * frame_decode/frame_encode викликаються тільки з main loop.
 */

#ifdef USE_COMM_ASYNC

#include "frame_codec.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/crc.h>

#include <stdint.h>
#include <stddef.h>

void frame_crc32_init(void)
{
    rcc_periph_clock_enable(RCC_CRC);
}

uint32_t frame_crc32(const uint8_t *data, size_t len)
{
    crc_reset();

    size_t i = 0;

    while (i + 4 <= len) {
        uint32_t word = ((uint32_t)data[i]     << 24) |
                        ((uint32_t)data[i + 1] << 16) |
                        ((uint32_t)data[i + 2] <<  8) |
                        ((uint32_t)data[i + 3]);
        crc_calculate(word);
        i += 4;
    }

    if (i < len) {
        uint32_t word  = 0;
        unsigned shift = 24;
        while (i < len) {
            word |= (uint32_t)data[i++] << shift;
            shift -= 8;
        }
        crc_calculate(word);
    }

    return CRC_DR;
}

#endif /* USE_COMM_ASYNC */
