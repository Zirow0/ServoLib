#include "./board_config.h"

#ifdef USE_COMM_ASYNC

#include "hwd_uart_async.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <string.h>

/* ================================================================
 * Таблиці: RCC / NVIC / GPIO AF
 * ================================================================ */

static enum rcc_periph_clken usart_rcc(uint32_t usart)
{
    if (usart == USART1) return RCC_USART1;
    if (usart == USART2) return RCC_USART2;
    return RCC_USART6;
}

static enum rcc_periph_clken dma_rcc(uint32_t dma)
{
    return (dma == DMA1) ? RCC_DMA1 : RCC_DMA2;
}

static enum rcc_periph_clken gpio_rcc(uint32_t port)
{
    if (port == GPIOA) return RCC_GPIOA;
    if (port == GPIOB) return RCC_GPIOB;
    if (port == GPIOC) return RCC_GPIOC;
    if (port == GPIOD) return RCC_GPIOD;
    return RCC_GPIOE;
}

/* USART1/2 = AF7, USART6 = AF8 */
static uint8_t usart_gpio_af(uint32_t usart)
{
    return (usart == USART6) ? GPIO_AF8 : GPIO_AF7;
}

static uint8_t usart_nvic_irq(uint32_t usart)
{
    if (usart == USART1) return NVIC_USART1_IRQ;
    if (usart == USART2) return NVIC_USART2_IRQ;
    return NVIC_USART6_IRQ;
}

static uint8_t dma_stream_nvic_irq(uint32_t dma, uint8_t stream)
{
    static const uint8_t dma1_irqs[8] = {
        NVIC_DMA1_STREAM0_IRQ, NVIC_DMA1_STREAM1_IRQ,
        NVIC_DMA1_STREAM2_IRQ, NVIC_DMA1_STREAM3_IRQ,
        NVIC_DMA1_STREAM4_IRQ, NVIC_DMA1_STREAM5_IRQ,
        NVIC_DMA1_STREAM6_IRQ, NVIC_DMA1_STREAM7_IRQ,
    };
    static const uint8_t dma2_irqs[8] = {
        NVIC_DMA2_STREAM0_IRQ, NVIC_DMA2_STREAM1_IRQ,
        NVIC_DMA2_STREAM2_IRQ, NVIC_DMA2_STREAM3_IRQ,
        NVIC_DMA2_STREAM4_IRQ, NVIC_DMA2_STREAM5_IRQ,
        NVIC_DMA2_STREAM6_IRQ, NVIC_DMA2_STREAM7_IRQ,
    };
    return (dma == DMA1) ? dma1_irqs[stream] : dma2_irqs[stream];
}

static uint32_t dma_chsel(uint8_t channel)
{
    static const uint32_t chsel[8] = {
        DMA_SxCR_CHSEL_0, DMA_SxCR_CHSEL_1, DMA_SxCR_CHSEL_2, DMA_SxCR_CHSEL_3,
        DMA_SxCR_CHSEL_4, DMA_SxCR_CHSEL_5, DMA_SxCR_CHSEL_6, DMA_SxCR_CHSEL_7,
    };
    return chsel[channel];
}

/* ================================================================
 * ISR dispatch
 * usart_inst: [0]=USART1, [1]=USART2, [2]=USART6
 * dma_tx_inst: [dma_idx 0=DMA1/1=DMA2][stream 0..7]
 * ================================================================ */

static uart_instance_t *usart_inst[3];
static uart_instance_t *dma_tx_inst[2][8];

static int usart_idx(uint32_t usart)
{
    if (usart == USART1) return 0;
    if (usart == USART2) return 1;
    if (usart == USART6) return 2;
    return -1;
}

/* ================================================================
 * DMA внутрішні функції
 * ================================================================ */

static void dma_rx_init(uart_instance_t *inst)
{
    const uart_hw_config_t *hw = inst->hw;

    dma_stream_reset(hw->rx_dma, hw->rx_stream);

    dma_set_peripheral_address(hw->rx_dma, hw->rx_stream,
                               (uint32_t)&USART_DR(hw->usart));
    dma_set_memory_address(hw->rx_dma, hw->rx_stream,
                           (uint32_t)inst->rx_buf);
    dma_set_number_of_data(hw->rx_dma, hw->rx_stream, COMM_RX_BUF_SIZE);

    dma_set_transfer_mode(hw->rx_dma, hw->rx_stream,
                          DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_dma_flow_control(hw->rx_dma, hw->rx_stream);
    dma_enable_circular_mode(hw->rx_dma, hw->rx_stream);

    dma_set_peripheral_size(hw->rx_dma, hw->rx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(hw->rx_dma, hw->rx_stream, DMA_SxCR_MSIZE_8BIT);
    dma_disable_peripheral_increment_mode(hw->rx_dma, hw->rx_stream);
    dma_enable_memory_increment_mode(hw->rx_dma, hw->rx_stream);

    dma_channel_select(hw->rx_dma, hw->rx_stream, dma_chsel(hw->rx_channel));
    dma_set_priority(hw->rx_dma, hw->rx_stream, DMA_SxCR_PL_HIGH);

    dma_enable_stream(hw->rx_dma, hw->rx_stream);
}

static void dma_tx_start(uart_instance_t *inst, uint8_t slot)
{
    const uart_hw_config_t *hw = inst->hw;

    dma_stream_reset(hw->tx_dma, hw->tx_stream);

    dma_set_peripheral_address(hw->tx_dma, hw->tx_stream,
                               (uint32_t)&USART_DR(hw->usart));
    dma_set_memory_address(hw->tx_dma, hw->tx_stream,
                           (uint32_t)inst->tx_pool[slot]);
    dma_set_number_of_data(hw->tx_dma, hw->tx_stream, inst->tx_len[slot]);

    dma_set_transfer_mode(hw->tx_dma, hw->tx_stream,
                          DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

    dma_set_peripheral_size(hw->tx_dma, hw->tx_stream, DMA_SxCR_PSIZE_8BIT);
    dma_set_memory_size(hw->tx_dma, hw->tx_stream, DMA_SxCR_MSIZE_8BIT);
    dma_disable_peripheral_increment_mode(hw->tx_dma, hw->tx_stream);
    dma_enable_memory_increment_mode(hw->tx_dma, hw->tx_stream);

    dma_channel_select(hw->tx_dma, hw->tx_stream, dma_chsel(hw->tx_channel));
    dma_set_priority(hw->tx_dma, hw->tx_stream, DMA_SxCR_PL_HIGH);
    dma_enable_transfer_complete_interrupt(hw->tx_dma, hw->tx_stream);

    dma_enable_stream(hw->tx_dma, hw->tx_stream);
}

/* ================================================================
 * Публічні функції
 * ================================================================ */

void uart_init(uart_instance_t *inst,
               const uart_hw_config_t *hw,
               uint32_t baud,
               uart_rx_cb_t cb)
{
    inst->hw          = hw;
    inst->rx_cb       = cb;
    inst->rx_last_pos = 0;
    inst->tx_head     = 0;
    inst->tx_tail     = 0;
    inst->tx_count    = 0;
    inst->tx_busy     = false;

    int ui = usart_idx(hw->usart);
    if (ui >= 0) usart_inst[ui] = inst;
    dma_tx_inst[(hw->tx_dma == DMA1) ? 0 : 1][hw->tx_stream] = inst;

    rcc_periph_clock_enable(dma_rcc(hw->rx_dma));
    rcc_periph_clock_enable(dma_rcc(hw->tx_dma));
    rcc_periph_clock_enable(gpio_rcc(hw->tx_port));
    rcc_periph_clock_enable(gpio_rcc(hw->rx_port));
    rcc_periph_clock_enable(usart_rcc(hw->usart));

    uint8_t af = usart_gpio_af(hw->usart);
    gpio_mode_setup(hw->tx_port, GPIO_MODE_AF, GPIO_PUPD_NONE, hw->tx_pin);
    gpio_set_af(hw->tx_port, af, hw->tx_pin);
    gpio_mode_setup(hw->rx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, hw->rx_pin);
    gpio_set_af(hw->rx_port, af, hw->rx_pin);

    usart_set_baudrate(hw->usart, baud);
    usart_set_databits(hw->usart, 8);
    usart_set_stopbits(hw->usart, USART_STOPBITS_1);
    usart_set_parity(hw->usart, USART_PARITY_NONE);
    usart_set_flow_control(hw->usart, USART_FLOWCONTROL_NONE);
    usart_set_mode(hw->usart, USART_MODE_TX_RX);
    usart_enable_rx_dma(hw->usart);
    usart_enable_tx_dma(hw->usart);
    USART_CR1(hw->usart) |= USART_CR1_IDLEIE;

    dma_rx_init(inst);

    uint8_t usart_irq  = usart_nvic_irq(hw->usart);
    uint8_t tx_dma_irq = dma_stream_nvic_irq(hw->tx_dma, hw->tx_stream);

    nvic_set_priority(usart_irq,  1);
    nvic_enable_irq(usart_irq);
    nvic_set_priority(tx_dma_irq, 2);
    nvic_enable_irq(tx_dma_irq);

    usart_enable(hw->usart);
}

bool uart_send(uart_instance_t *inst, const uint8_t *data, size_t len)
{
    if (len == 0 || len > COMM_TX_BUF_SIZE) return false;

    uint8_t tx_dma_irq = dma_stream_nvic_irq(inst->hw->tx_dma,
                                              inst->hw->tx_stream);
    nvic_disable_irq(tx_dma_irq);

    if (inst->tx_count >= COMM_TX_QUEUE_LEN) {
        nvic_enable_irq(tx_dma_irq);
        return false;
    }

    memcpy(inst->tx_pool[inst->tx_tail], data, len);
    inst->tx_len[inst->tx_tail] = (uint8_t)len;
    inst->tx_tail = (inst->tx_tail + 1) % COMM_TX_QUEUE_LEN;
    inst->tx_count++;

    if (!inst->tx_busy) {
        inst->tx_busy = true;
        uint8_t slot = inst->tx_head;
        inst->tx_head = (inst->tx_head + 1) % COMM_TX_QUEUE_LEN;
        inst->tx_count--;
        nvic_enable_irq(tx_dma_irq);
        dma_tx_start(inst, slot);
    } else {
        nvic_enable_irq(tx_dma_irq);
    }

    return true;
}

/* ================================================================
 * ISR обробники
 * ================================================================ */

static void uart_idle_handler(uart_instance_t *inst)
{
    const uart_hw_config_t *hw = inst->hw;

    if (!(USART_SR(hw->usart) & USART_SR_IDLE)) return;

    /* Очистити IDLE: читання SR, потім DR */
    (void)USART_SR(hw->usart);
    (void)USART_DR(hw->usart);

    size_t current_pos = COMM_RX_BUF_SIZE
                       - DMA_SNDTR(hw->rx_dma, hw->rx_stream);

    if (current_pos != inst->rx_last_pos && inst->rx_cb) {
        bool accepted;
        if (current_pos > inst->rx_last_pos) {
            accepted = inst->rx_cb(inst->rx_buf + inst->rx_last_pos,
                                   current_pos - inst->rx_last_pos);
        } else {
            /* wrap-around: лінеаризуємо в тимчасовий буфер */
            size_t  part1 = COMM_RX_BUF_SIZE - inst->rx_last_pos;
            size_t  part2 = current_pos;
            uint8_t tmp[COMM_RX_BUF_SIZE];
            memcpy(tmp,         inst->rx_buf + inst->rx_last_pos, part1);
            memcpy(tmp + part1, inst->rx_buf,                     part2);
            accepted = inst->rx_cb(tmp, part1 + part2);
        }
        /* Просуваємо rx_last_pos лише якщо callback прийняв дані.
         * Інакше наступний IDLE повторно доставить той самий фрейм. */
        if (accepted) {
            inst->rx_last_pos = current_pos;
        }
    }
}

static void uart_dma_tx_handler(uart_instance_t *inst)
{
    const uart_hw_config_t *hw = inst->hw;

    if (!dma_get_interrupt_flag(hw->tx_dma, hw->tx_stream, DMA_TCIF)) return;
    dma_clear_interrupt_flags(hw->tx_dma, hw->tx_stream, DMA_TCIF);

    if (inst->tx_count > 0) {
        uint8_t slot = inst->tx_head;
        inst->tx_head = (inst->tx_head + 1) % COMM_TX_QUEUE_LEN;
        inst->tx_count--;
        dma_tx_start(inst, slot);
    } else {
        inst->tx_busy = false;
    }
}

/* ================================================================
 * ISR векторів
 * USART1 TX: DMA2 Stream7 Ch4
 * USART2 TX: DMA1 Stream6 Ch4
 * USART6 TX: DMA2 Stream6 Ch5 або DMA2 Stream7 Ch5
 * ================================================================ */

void usart1_isr(void) { if (usart_inst[0]) uart_idle_handler(usart_inst[0]); }
void usart2_isr(void) { if (usart_inst[1]) uart_idle_handler(usart_inst[1]); }
void usart6_isr(void) { if (usart_inst[2]) uart_idle_handler(usart_inst[2]); }

void dma1_stream6_isr(void)
{
    uart_instance_t *i = dma_tx_inst[0][6];
    if (i) uart_dma_tx_handler(i);
}

void dma2_stream6_isr(void)
{
    uart_instance_t *i = dma_tx_inst[1][6];
    if (i) uart_dma_tx_handler(i);
}

void dma2_stream7_isr(void)
{
    uart_instance_t *i = dma_tx_inst[1][7];
    if (i) uart_dma_tx_handler(i);
}

#endif /* USE_COMM_ASYNC */
