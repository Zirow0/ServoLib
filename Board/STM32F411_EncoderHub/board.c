/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include <libopencm3/stm32/common/flash_common_idcache.h>

/* Private functions ---------------------------------------------------------*/

/* HSE=25 MHz, PLL → SYSCLK=100 MHz
 * APB1=50 MHz (таймери APB1 = 100 MHz ×2), APB2=100 MHz */
static void clock_setup(void)
{
    const struct rcc_clock_scale hse25_100mhz = {
        .pllm          = 25,
        .plln          = 200,
        .pllp          = 2,
        .pllq          = 4,
        .pllr          = 0,
        .pll_source    = RCC_CFGR_PLLSRC_HSE_CLK,
        .hpre          = RCC_CFGR_HPRE_NODIV,
        .ppre1         = RCC_CFGR_PPRE_DIV2,
        .ppre2         = RCC_CFGR_PPRE_NODIV,
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

    /* DRDY — PB0, вихід push-pull, за замовчуванням LOW */
    gpio_mode_setup(DRDY_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DRDY_PIN);
    gpio_set_output_options(DRDY_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, DRDY_PIN);
    gpio_clear(DRDY_GPIO_PORT, DRDY_PIN);
}

/* Всі 12 пінів енкодерів — вхід з підтяжкою до живлення */
static void encoder_gpio_setup(void)
{
    /* PA0-PA7: ENC0_A, ENC0_B, ENC1_A, ENC1_B, ENC2_A, ENC2_B, ENC3_A, ENC3_B */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                    GPIO0 | GPIO1 | GPIO2 | GPIO3 |
                    GPIO4 | GPIO5 | GPIO6 | GPIO7);

    /* PA8: ENC4_A, PA11: ENC5_B */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                    GPIO8 | GPIO11);

    /* PB9: ENC4_B, PB10: ENC5_A */
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                    GPIO9 | GPIO10);
}

/* SPI2 slave: PB12=NSS, PB13=SCK, PB14=MISO, PB15=MOSI (AF5) */
static void spi_slave_gpio_setup(void)
{
    rcc_periph_clock_enable(HUB_SPI_RCC);

    gpio_mode_setup(HUB_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    HUB_SPI_NSS_PIN | HUB_SPI_SCK_PIN |
                    HUB_SPI_MISO_PIN | HUB_SPI_MOSI_PIN);

    /* Тільки MISO — вихід; решта — входи від master */
    gpio_set_output_options(HUB_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            HUB_SPI_MISO_PIN);

    gpio_set_af(HUB_SPI_GPIO_PORT, HUB_SPI_GPIO_AF,
                HUB_SPI_NSS_PIN | HUB_SPI_SCK_PIN |
                HUB_SPI_MISO_PIN | HUB_SPI_MOSI_PIN);
}

#ifdef USE_HWD_UART
static void uart_setup(void)
{
    rcc_periph_clock_enable(UART_DEBUG_RCC);

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

/* TIM2 (32-bit): free-running µs counter для вимірювання period_us в EXTI ISR */
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

/* TIM5: 10 kHz interrupt — тригер UKF update для 6 енкодерів.
 * Викликати з App після ініціалізації всіх енкодерів та UKF. */
void Board_StartUpdateTimer(void)
{
    rcc_periph_clock_enable(UPDATE_TIMER_RCC);

    timer_set_mode(UPDATE_TIMER,
                   TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE,
                   TIM_CR1_DIR_UP);

    timer_set_prescaler(UPDATE_TIMER, UPDATE_TIMER_PRESCALER);
    timer_set_period(UPDATE_TIMER, UPDATE_TIMER_PERIOD);

    timer_enable_irq(UPDATE_TIMER, TIM_DIER_UIE);
    nvic_enable_irq(UPDATE_TIMER_IRQ);

    timer_enable_counter(UPDATE_TIMER);
}

static void systick_setup(void)
{
    systick_set_frequency(SYSTICK_FREQ, SYSTEM_CORE_CLOCK);
    systick_counter_enable();
    systick_interrupt_enable();
}

/* Public API ----------------------------------------------------------------*/

Servo_Status_t Board_Init(void)
{
    clock_setup();
    gpio_rcc_setup();
    gpio_misc_setup();
    encoder_gpio_setup();
    spi_slave_gpio_setup();
    micros_timer_setup();
    systick_setup();

#ifdef USE_HWD_UART
    uart_setup();
#endif

    return SERVO_OK;
}
