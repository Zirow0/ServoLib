/**
 * @file board.c
 * @brief Ініціалізація апаратного забезпечення STM32F411 (libopencm3)
 * @author ServoCore Team
 * @date 2025
 *
 * Виконує всю ініціалізацію периферії замість CubeMX:
 *   1. Системний клок 100 MHz (HSE 25 MHz → PLL)
 *   2. RCC clock enables для всіх використовуваних периферій
 *   3. GPIO alternate functions (TIM3 PWM, SPI1)
 *   4. TIM5 — мікросекундний таймер (32-bit, 1 MHz)
 *   5. SPI1 — для AEAT-9922 (MODE1: CPOL=0, CPHA=1, 8-bit, MSB first)
 *   6. I2C1 — для AS5600 (умовно, #ifdef USE_HWD_I2C)
 *   7. SysTick — 1 kHz для g_uptime_ms
 *
 * Конфлікт пінів PA6/PA7:
 *   PA6 і PA7 використовуються і TIM3 (AF2), і SPI1 (AF5).
 *   Одночасно активними бути не можуть. Board_Init() налаштовує
 *   піни відповідно до активних макросів USE_MOTOR_PWM / USE_HWD_SPI.
 *   Якщо увімкнено обидва — пріоритет має USE_HWD_SPI (поточна конфігурація).
 *
 * Виклик: один раз на початку main() до будь-якого HWD драйвера.
 */

/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include "../../Inc/core.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/common/flash_common_idcache.h>  /* FLASH_ACR_DCEN, FLASH_ACR_ICEN */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Налаштування системного клоку 100 MHz через HSE 25 MHz + PLL.
 *
 * STM32F411 BlackPill: HSE = 25 MHz кварц.
 * PLL: M=25, N=200, P=2 → VCO=200 MHz, SYSCLK=100 MHz
 * APB1 prescaler=2 → APB1=50 MHz (таймери APB1 = 100 MHz завдяки ×2)
 * APB2 prescaler=1 → APB2=100 MHz
 */
static void clock_setup(void)
{
    /*
     * libopencm3 надає готову конфігурацію для STM32F411 @ 100 MHz
     * з HSE 25 MHz через rcc_clock_setup_pll().
     */
    const struct rcc_clock_scale hse25_100mhz = {
        .pllm       = 25,
        .plln       = 200,
        .pllp       = 2,
        .pllq       = 4,
        .pllr       = 0,
        .pll_source = RCC_CFGR_PLLSRC_HSE_CLK,
        .hpre       = RCC_CFGR_HPRE_NODIV,     /* AHB  = 100 MHz */
        .ppre1      = RCC_CFGR_PPRE_DIV2,       /* APB1 = 50 MHz  */
        .ppre2      = RCC_CFGR_PPRE_NODIV,      /* APB2 = 100 MHz */
        .voltage_scale = PWR_SCALE1,
        .flash_config  = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_LATENCY_3WS,
        .ahb_frequency  = 100000000U,
        .apb1_frequency = 50000000U,
        .apb2_frequency = 100000000U,
    };

    rcc_clock_setup_pll(&hse25_100mhz);
}

/**
 * @brief Увімкнення тактування всіх використовуваних GPIO портів.
 */
static void gpio_rcc_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);   /* PA4..PA8, PA5/PA6/PA7 SPI+PWM */
    rcc_periph_clock_enable(RCC_GPIOB);   /* PB0 MSEL, PB6/PB7 I2C */
    rcc_periph_clock_enable(RCC_GPIOC);   /* PC13 LED */
}

/**
 * @brief Ініціалізація GPIO пінів не пов'язаних з SPI/I2C/PWM.
 */
static void gpio_misc_setup(void)
{
    /* LED — PC13, вихід push-pull */
    gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_set_output_options(LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_PIN);

    /* Brake — PA8, вихід push-pull, за замовчуванням LOW (гальмо увімкнено) */
    gpio_mode_setup(BRAKE_CTRL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BRAKE_CTRL_PIN);
    gpio_set_output_options(BRAKE_CTRL_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, BRAKE_CTRL_PIN);
    gpio_clear(BRAKE_CTRL_GPIO_PORT, BRAKE_CTRL_PIN);
}

/**
 * @brief Ініціалізація TIM3 для PWM (якщо USE_MOTOR_PWM і !USE_HWD_SPI).
 *
 * PA6 (CH1) і PA7 (CH2) конфліктують з SPI1.
 * Якщо USE_HWD_SPI активний — ці піни займає SPI, PWM недоступний.
 */
#if defined(USE_MOTOR_PWM) && !defined(USE_HWD_SPI)
static void pwm_gpio_setup(void)
{
    gpio_mode_setup(MOTOR_PWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR_PWM_GPIO_CH1 | MOTOR_PWM_GPIO_CH2);
    gpio_set_output_options(MOTOR_PWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ,
                            MOTOR_PWM_GPIO_CH1 | MOTOR_PWM_GPIO_CH2);
    gpio_set_af(MOTOR_PWM_GPIO_PORT, MOTOR_PWM_GPIO_AF,
                MOTOR_PWM_GPIO_CH1 | MOTOR_PWM_GPIO_CH2);
}
#endif /* USE_MOTOR_PWM && !USE_HWD_SPI */

/**
 * @brief Ініціалізація SPI1 для AEAT-9922.
 *
 * AEAT-9922 SPI режим: CPOL=0, CPHA=1 (MODE 1), 8-bit, MSB first, ~1 MHz.
 * CS (PA4) керується вручну через GPIO.
 * MSEL (PB0) — вихід для вибору режиму роботи AEAT-9922.
 */
#ifdef USE_HWD_SPI
static void spi_setup(void)
{
    rcc_periph_clock_enable(ENCODER_SPI_RCC);

    /* SPI1 GPIO: PA5 SCK, PA6 MISO, PA7 MOSI — AF5 */
    gpio_mode_setup(ENCODER_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    ENCODER_SPI_SCK | ENCODER_SPI_MISO | ENCODER_SPI_MOSI);
    gpio_set_output_options(ENCODER_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            ENCODER_SPI_SCK | ENCODER_SPI_MISO | ENCODER_SPI_MOSI);
    gpio_set_af(ENCODER_SPI_GPIO_PORT, ENCODER_SPI_GPIO_AF,
                ENCODER_SPI_SCK | ENCODER_SPI_MISO | ENCODER_SPI_MOSI);

    /* CS — PA4, вихід, початково HIGH (неактивний) */
    gpio_mode_setup(ENCODER_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ENCODER_CS_PIN);
    gpio_set_output_options(ENCODER_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, ENCODER_CS_PIN);
    gpio_set(ENCODER_CS_GPIO_PORT, ENCODER_CS_PIN);

    /* MSEL — PB0, вихід, HIGH = SPI4-wire 24-bit mode */
    gpio_mode_setup(ENCODER_MSEL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ENCODER_MSEL_PIN);
    gpio_set_output_options(ENCODER_MSEL_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, ENCODER_MSEL_PIN);
    gpio_set(ENCODER_MSEL_GPIO_PORT, ENCODER_MSEL_PIN);

    /*
     * SPI1 ініціалізація:
     *   CPOL=0 (clock idle LOW), CPHA=1 (data on falling edge) → MODE 1
     *   Baudrate: APB2 (100 MHz) / 128 ≈ 781 kHz (AEAT max ~2 MHz)
     *   8-bit, MSB first, software NSS
     */
    spi_init_master(ENCODER_SPI,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_128,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_2,
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);

    spi_enable_software_slave_management(ENCODER_SPI);
    spi_set_nss_high(ENCODER_SPI);
    spi_enable(ENCODER_SPI);
}
#endif /* USE_HWD_SPI */

/**
 * @brief Ініціалізація I2C1 для AS5600 (якщо USE_HWD_I2C).
 */
#ifdef USE_HWD_I2C
static void i2c_setup(void)
{
    rcc_periph_clock_enable(SENSOR_I2C_RCC);

    /* PB6 SCL, PB7 SDA — AF4, open-drain з підтяжкою */
    gpio_mode_setup(SENSOR_I2C_GPIO_PORT, GPIO_MODE_AF,
                    GPIO_PUPD_PULLUP,
                    SENSOR_I2C_SCL | SENSOR_I2C_SDA);
    gpio_set_output_options(SENSOR_I2C_GPIO_PORT, GPIO_OTYPE_OD,
                            GPIO_OSPEED_25MHZ,
                            SENSOR_I2C_SCL | SENSOR_I2C_SDA);
    gpio_set_af(SENSOR_I2C_GPIO_PORT, SENSOR_I2C_GPIO_AF,
                SENSOR_I2C_SCL | SENSOR_I2C_SDA);

    i2c_reset(SENSOR_I2C);
    i2c_peripheral_disable(SENSOR_I2C);

    /* 400 kHz Fast Mode, APB1 = 50 MHz */
    i2c_set_speed(SENSOR_I2C, i2c_speed_fm_400k, SENSOR_I2C_APB_MHZ);

    i2c_peripheral_enable(SENSOR_I2C);
}
#endif /* USE_HWD_I2C */

/**
 * @brief Ініціалізація TIM5 як мікросекундного таймера.
 *
 * 32-bit таймер, prescaler=99 → 1 MHz (1 тік = 1 мкс).
 * Переповнення через ~71 хвилин — безпечно для вимірювань коротких інтервалів.
 */
static void micros_timer_setup(void)
{
    rcc_periph_clock_enable(MICROS_TIMER_RCC);

    timer_set_mode(MICROS_TIMER,
                   TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE,
                   TIM_CR1_DIR_UP);

    /* APB1 timer clock = 100 MHz (×2 від APB1=50 MHz) */
    timer_set_prescaler(MICROS_TIMER, MICROS_TIMER_PRESCALER);  /* 100-1=99 */
    timer_set_period(MICROS_TIMER, 0xFFFFFFFFU);                 /* 32-bit max */

    timer_enable_counter(MICROS_TIMER);
}

/**
 * @brief Налаштування SysTick для генерації 1 ms переривань.
 *
 * sys_tick_handler() в hwd_timer.c інкрементує g_uptime_ms.
 */
static void systick_setup(void)
{
    systick_set_frequency(SYSTICK_FREQ, SYSTEM_CORE_CLOCK);
    systick_counter_enable();
    systick_interrupt_enable();
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Board_Init(void)
{
    clock_setup();
    gpio_rcc_setup();
    gpio_misc_setup();

#if defined(USE_MOTOR_PWM) && !defined(USE_HWD_SPI)
    pwm_gpio_setup();
    rcc_periph_clock_enable(MOTOR_PWM_TIMER_RCC);
#endif

#ifdef USE_HWD_SPI
    spi_setup();
#endif

#ifdef USE_HWD_I2C
    i2c_setup();
#endif

    micros_timer_setup();
    systick_setup();

    return SERVO_OK;
}

Servo_Status_t Board_DeInit(void)
{
    systick_interrupt_disable();
    systick_counter_disable();

    timer_disable_counter(MICROS_TIMER);

#ifdef USE_HWD_SPI
    spi_disable(ENCODER_SPI);
#endif

#ifdef USE_HWD_I2C
    i2c_peripheral_disable(SENSOR_I2C);
#endif

    return SERVO_OK;
}
