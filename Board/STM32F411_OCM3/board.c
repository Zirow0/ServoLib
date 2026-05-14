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

#ifdef USE_MOTOR_PWM
    /* Motor DIR — PA7, вихід push-pull, за замовчуванням LOW */
    gpio_mode_setup(MOTOR_DIR_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MOTOR_DIR_PIN);
    gpio_set_output_options(MOTOR_DIR_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, MOTOR_DIR_PIN);
    gpio_clear(MOTOR_DIR_GPIO_PORT, MOTOR_DIR_PIN);
#endif
}

#ifdef USE_MOTOR_PWM
static void pwm_gpio_setup(void)
{
    gpio_mode_setup(MOTOR_PWM_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR_PWM_GPIO_CH1);
    gpio_set_output_options(MOTOR_PWM_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            MOTOR_PWM_GPIO_CH1);
    gpio_set_af(MOTOR_PWM_GPIO_PORT, MOTOR_PWM_GPIO_AF,
                MOTOR_PWM_GPIO_CH1);
}
#endif /* USE_MOTOR_PWM */

#if defined(USE_HWD_UART) && !defined(USE_COMM_ASYNC)
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
#endif /* USE_HWD_UART && !USE_COMM_ASYNC */

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

#ifdef USE_MOTOR_PWM
    pwm_gpio_setup();
    rcc_periph_clock_enable(MOTOR_PWM_TIMER_RCC);
#endif

    micros_timer_setup();
    systick_setup();

#if defined(USE_HWD_UART) && !defined(USE_COMM_ASYNC)
    uart_setup();
#endif

    return SERVO_OK;
}
