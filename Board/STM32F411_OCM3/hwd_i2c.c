/**
 * @file hwd_i2c.c
 * @brief Реалізація HWD I2C для STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * I2C абстракція через libopencm3.
 *
 * Зберігання в HWD_I2C_Config_t:
 *   hw_handle → (void*)(uintptr_t)I2C1 — базова адреса I2C
 *
 * Адресація:
 *   Інтерфейс приймає 8-bit адресу (як HAL): dev_address = 7-bit addr << 1
 *   libopencm3 i2c_transfer7() очікує 7-bit адресу → ділимо на 2.
 *
 * Передумова: Board_Init() вже ініціалізував I2C та GPIO.
 *
 * Обмеження WriteReg:
 *   Тимчасовий буфер на стеку (регістр + дані). Не використовувати
 *   для size > 64 байт — розгляньте статичний буфер при потребі.
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"

#ifdef USE_HWD_I2C

#include "hwd/hwd_i2c.h"
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/nvic.h>

/* Private functions ---------------------------------------------------------*/

static inline uint8_t to_7bit(uint16_t addr_8bit)
{
    return (uint8_t)(addr_8bit >> 1);
}

/* Continuous read state machine ----------------------------------------------
 *
 * Підтримує size = 1 або 2 байти з repeated start.
 * Для 2 байтів використовується POS bit (STM32F4 RM0383 §27.3.3).
 *
 * Цикл: START → addr+W → reg → rSTART → addr+R → data → [restart]
 */

typedef enum {
    CT_IDLE    = 0,
    CT_WR_ADDR,   /* START надіслано → надіслати dev+WRITE */
    CT_WR_REG,    /* ADDR підтверджено → надіслати reg_addr */
    CT_RD_ADDR,   /* repeated START → надіслати dev+READ */
    CT_RD_DATA,   /* ADDR підтверджено → отримати байти */
} CT_Phase_t;

#define CT_SIZE_MAX  2U

static struct {
    uint32_t          i2c;
    uint8_t           dev_7bit;
    uint8_t           reg_addr;
    volatile uint8_t* buf;
    uint8_t           size;
    CT_Phase_t        phase;
    uint8_t           tmp[CT_SIZE_MAX];
} s_ct;

void HWD_I2C_EV_Handler(void)
{
    if (s_ct.phase == CT_IDLE) return;

    uint32_t sr1 = I2C_SR1(s_ct.i2c);
    uint32_t sr2;

    /* EV5: START bit надіслано */
    if (sr1 & I2C_SR1_SB) {
        if (s_ct.phase == CT_WR_ADDR) {
            i2c_send_7bit_address(s_ct.i2c, s_ct.dev_7bit, I2C_WRITE);
            s_ct.phase = CT_WR_REG;
        } else { /* CT_RD_ADDR */
            i2c_send_7bit_address(s_ct.i2c, s_ct.dev_7bit, I2C_READ);
            s_ct.phase = CT_RD_DATA;
        }
        return;
    }

    /* EV6: адреса підтверджена */
    if (sr1 & I2C_SR1_ADDR) {
        if (s_ct.phase == CT_WR_REG) {
            sr2 = I2C_SR2(s_ct.i2c); (void)sr2;
        } else { /* CT_RD_DATA */
            if (s_ct.size == 1) {
                i2c_disable_ack(s_ct.i2c);
                sr2 = I2C_SR2(s_ct.i2c); (void)sr2;
                i2c_send_stop(s_ct.i2c);
            } else { /* size == 2: POS mode */
                I2C_CR1(s_ct.i2c) |= I2C_CR1_POS;
                i2c_disable_ack(s_ct.i2c);
                sr2 = I2C_SR2(s_ct.i2c); (void)sr2;
            }
        }
        return;
    }

    /* EV8_2 / EV7_1: BTF */
    if (sr1 & I2C_SR1_BTF) {
        if (s_ct.phase == CT_WR_REG) {
            /* reg_addr передано → repeated START */
            i2c_send_start(s_ct.i2c);
            s_ct.phase = CT_RD_ADDR;
        } else { /* CT_RD_DATA, size == 2: обидва байти готові */
            i2c_send_stop(s_ct.i2c);
            s_ct.tmp[0] = (uint8_t)i2c_get_data(s_ct.i2c);
            s_ct.tmp[1] = (uint8_t)i2c_get_data(s_ct.i2c);
            I2C_CR1(s_ct.i2c) &= ~I2C_CR1_POS;
            s_ct.buf[0] = s_ct.tmp[0];
            s_ct.buf[1] = s_ct.tmp[1];
            i2c_enable_ack(s_ct.i2c);
            s_ct.phase = CT_WR_ADDR;
            i2c_send_start(s_ct.i2c);
        }
        return;
    }

    /* EV8: TXE — надіслати адресу регістру */
    if ((sr1 & I2C_SR1_TxE) && s_ct.phase == CT_WR_REG) {
        i2c_send_data(s_ct.i2c, s_ct.reg_addr);
        return;
    }

    /* EV7: RXNE — тільки для size == 1 */
    if ((sr1 & I2C_SR1_RxNE) && s_ct.phase == CT_RD_DATA && s_ct.size == 1) {
        s_ct.tmp[0] = (uint8_t)i2c_get_data(s_ct.i2c);
        s_ct.buf[0] = s_ct.tmp[0];
        s_ct.phase  = CT_WR_ADDR;
        i2c_send_start(s_ct.i2c);
    }
}

void i2c1_ev_isr(void)
{
    HWD_I2C_EV_Handler();
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t HWD_I2C_Init(HWD_I2C_Handle_t* handle, const HWD_I2C_Config_t* config)
{
    if (handle == NULL || config == NULL || config->hw_handle == NULL) {
        return SERVO_INVALID;
    }

    handle->config = *config;
    return SERVO_OK;
}

Servo_Status_t HWD_I2C_ReadReg(HWD_I2C_Handle_t* handle,
                                uint16_t          dev_address,
                                uint8_t           reg_address,
                                uint8_t*          data,
                                uint16_t          size)
{
    if (handle == NULL || handle->config.hw_handle == NULL || data == NULL || size == 0) {
        return SERVO_INVALID;
    }

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    i2c_transfer7(i2c, to_7bit(dev_address), &reg_address, 1, data, size);

    return SERVO_OK;
}

Servo_Status_t HWD_I2C_ReadRegByte(HWD_I2C_Handle_t* handle,
                                    uint16_t          dev_address,
                                    uint8_t           reg_address,
                                    uint8_t*          value)
{
    if (value == NULL) return SERVO_INVALID;
    return HWD_I2C_ReadReg(handle, dev_address, reg_address, value, 1);
}

Servo_Status_t HWD_I2C_IsDeviceReady(HWD_I2C_Handle_t* handle,
                                      uint16_t          dev_address,
                                      uint8_t           trials)
{
    if (handle == NULL || handle->config.hw_handle == NULL) {
        return SERVO_INVALID;
    }

    if (trials == 0) trials = 1;

    uint32_t i2c   = (uint32_t)(uintptr_t)handle->config.hw_handle;
    uint8_t  dummy = 0;

    for (uint8_t i = 0; i < trials; i++) {
        i2c_transfer7(i2c, to_7bit(dev_address), NULL, 0, &dummy, 1);

        if (!(I2C_SR1(i2c) & I2C_SR1_AF)) {
            return SERVO_OK;
        }

        I2C_SR1(i2c) &= ~I2C_SR1_AF;
    }

    return SERVO_ERROR;
}

Servo_Status_t HWD_I2C_StartContinuousRead(HWD_I2C_Handle_t* handle,
                                            uint16_t          dev_address,
                                            uint8_t           reg_address,
                                            volatile uint8_t* buf,
                                            uint16_t          size)
{
    if (handle == NULL || handle->config.hw_handle == NULL) return SERVO_INVALID;
    if (buf == NULL || size == 0 || size > CT_SIZE_MAX)      return SERVO_INVALID;

    uint32_t i2c = (uint32_t)(uintptr_t)handle->config.hw_handle;

    s_ct.i2c      = i2c;
    s_ct.dev_7bit = to_7bit(dev_address);
    s_ct.reg_addr = reg_address;
    s_ct.buf      = buf;
    s_ct.size     = (uint8_t)size;
    s_ct.phase    = CT_WR_ADDR;

    i2c_enable_ack(i2c);
    nvic_enable_irq(NVIC_I2C1_EV_IRQ);
    i2c_enable_interrupt(i2c, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
    i2c_send_start(i2c);

    return SERVO_OK;
}

#endif /* USE_HWD_I2C */
