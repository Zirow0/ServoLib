/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include <libopencm3/stm32/common/flash_common_idcache.h>

/* Private functions ---------------------------------------------------------*/

/* STM32F411 BlackPill: HSE=25 MHz, PLL → SYSCLK=100 MHz
 * APB1=50 MHz (таймери APB1 = 100 MHz ×2), APB2=100 MHz */
static void clock_setup(void)
{
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

static void gpio_rcc_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
}

static void gpio_misc_setup(void)
{
    /* LED — PC13, вихід push-pull */
    gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_set_output_options(LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_PIN);

    /* Brake — PA8, вихід push-pull, за замовчуванням LOW (гальмо увімкнено) */
    gpio_mode_setup(BRAKE_CTRL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BRAKE_CTRL_PIN);
    gpio_set_output_options(BRAKE_CTRL_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, BRAKE_CTRL_PIN);
    gpio_clear(BRAKE_CTRL_GPIO_PORT, BRAKE_CTRL_PIN);

#if defined(USE_MOTOR_PWM) && !defined(USE_HWD_SPI)
    /* Motor DIR — PA7, вихід push-pull, за замовчуванням LOW */
    gpio_mode_setup(MOTOR_DIR_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MOTOR_DIR_PIN);
    gpio_set_output_options(MOTOR_DIR_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, MOTOR_DIR_PIN);
    gpio_clear(MOTOR_DIR_GPIO_PORT, MOTOR_DIR_PIN);
#endif
}

/* PA6 конфліктує з SPI1 (AF5) — одночасно не можна */
#if defined(USE_MOTOR_PWM) && !defined(USE_HWD_SPI)
static void pwm_gpio_setup(void)
{
    gpio_mode_setup(MOTOR_PWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR_PWM_GPIO_CH1);
    gpio_set_output_options(MOTOR_PWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            MOTOR_PWM_GPIO_CH1);
    gpio_set_af(MOTOR_PWM_GPIO_PORT, MOTOR_PWM_GPIO_AF,
                MOTOR_PWM_GPIO_CH1);
}
#endif /* USE_MOTOR_PWM && !USE_HWD_SPI */

#ifdef USE_HWD_SPI
static void spi_setup(void)
{
    rcc_periph_clock_enable(ENCODER_SPI_RCC);
    rcc_periph_clock_enable(ENCODER_SPI_DATA_RCC);

    /* PA5 → SCK */
    gpio_mode_setup(ENCODER_SPI_SCK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, ENCODER_SPI_SCK);
    gpio_set_output_options(ENCODER_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, ENCODER_SPI_SCK);
    gpio_set_af(ENCODER_SPI_SCK_PORT, ENCODER_SPI_GPIO_AF, ENCODER_SPI_SCK);

    /* PB4 → MISO, PB5 → MOSI */
    gpio_mode_setup(ENCODER_SPI_DATA_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    ENCODER_SPI_MISO | ENCODER_SPI_MOSI);
    gpio_set_output_options(ENCODER_SPI_DATA_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            ENCODER_SPI_MISO | ENCODER_SPI_MOSI);
    gpio_set_af(ENCODER_SPI_DATA_PORT, ENCODER_SPI_GPIO_AF,
                ENCODER_SPI_MISO | ENCODER_SPI_MOSI);

    /* CS — PA4, вихід, початково HIGH (неактивний) */
    gpio_mode_setup(ENCODER_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ENCODER_CS_PIN);
    gpio_set_output_options(ENCODER_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, ENCODER_CS_PIN);
    gpio_set(ENCODER_CS_GPIO_PORT, ENCODER_CS_PIN);

    /* MSEL — PB0, вихід, HIGH = SPI4-wire 24-bit mode */
    gpio_mode_setup(ENCODER_MSEL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ENCODER_MSEL_PIN);
    gpio_set_output_options(ENCODER_MSEL_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, ENCODER_MSEL_PIN);
    gpio_set(ENCODER_MSEL_GPIO_PORT, ENCODER_MSEL_PIN);

    /* CPOL=0, CPHA=1 (MODE 1), APB2/128 ≈ 781 kHz, 8-bit MSB first */
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

#ifdef USE_HWD_UART
static void uart_setup(void)
{
    rcc_periph_clock_enable(UART_DEBUG_RCC);

    /* PA9 TX, PA10 RX — AF7, push-pull */
    gpio_mode_setup(UART_DEBUG_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    UART_DEBUG_TX_PIN | UART_DEBUG_RX_PIN);
    gpio_set_output_options(UART_DEBUG_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            UART_DEBUG_TX_PIN | UART_DEBUG_RX_PIN);
    gpio_set_af(UART_DEBUG_GPIO_PORT, UART_DEBUG_GPIO_AF,
                UART_DEBUG_TX_PIN | UART_DEBUG_RX_PIN);

    usart_set_baudrate(UART_DEBUG, UART_DEBUG_BAUDRATE);
    usart_set_databits(UART_DEBUG, 8);
    usart_set_stopbits(UART_DEBUG, USART_STOPBITS_1);
    usart_set_parity(UART_DEBUG, USART_PARITY_NONE);
    usart_set_flow_control(UART_DEBUG, USART_FLOWCONTROL_NONE);
    usart_set_mode(UART_DEBUG, USART_MODE_TX_RX);
    usart_enable(UART_DEBUG);
}
#endif /* USE_HWD_UART */

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

static void micros_timer_setup(void)
{
    rcc_periph_clock_enable(MICROS_TIMER_RCC);

    timer_set_mode(MICROS_TIMER,
                   TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE,
                   TIM_CR1_DIR_UP);

    timer_set_prescaler(MICROS_TIMER, MICROS_TIMER_PRESCALER);
    timer_set_period(MICROS_TIMER, 0xFFFFFFFFU);

    timer_enable_counter(MICROS_TIMER);
}

static void systick_setup(void)
{
    systick_set_frequency(SYSTICK_FREQ, SYSTEM_CORE_CLOCK);
    systick_counter_enable();
    systick_interrupt_enable();
}

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

#ifdef USE_HWD_UART
    uart_setup();
#endif

    return SERVO_OK;
}
