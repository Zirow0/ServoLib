/* Includes ------------------------------------------------------------------*/
#include "./board_config.h"
#include <libopencm3/stm32/common/flash_common_idcache.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>

/* Exported data -------------------------------------------------------------*/
volatile bool g_estop_active = false;

/* Private functions ---------------------------------------------------------*/

/* HSE=25 MHz, PLL → SYSCLK=100 MHz
 * APB1=50 MHz (таймери APB1 = 100 MHz × 2), APB2=100 MHz */
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
        .apb1_frequency =  50000000U,
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

static void led_gpio_setup(void)
{
    gpio_mode_setup(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_set_output_options(LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_PIN);
}

/* Всі 6 гальм — вихід push-pull, LOW при старті = гальмо увімкнено (fail-safe).
 * Виконується до ініціалізації будь-якого App-коду. */
static void brake_gpio_setup(void)
{
    /* PB9, PB10 */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    BRAKE0_PIN | BRAKE1_PIN);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ,
                            BRAKE0_PIN | BRAKE1_PIN);
    gpio_clear(GPIOB, BRAKE0_PIN | BRAKE1_PIN);

    /* PA12, PA15 */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    BRAKE2_PIN | BRAKE3_PIN);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ,
                            BRAKE2_PIN | BRAKE3_PIN);
    gpio_clear(GPIOA, BRAKE2_PIN | BRAKE3_PIN);

    /* PB2 (BRAKE4 = BOOT1): pull-down на платі вже тримає LOW.
     * Конфігуруємо як вихід явно для гарантованого керування. */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BRAKE4_PIN);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, BRAKE4_PIN);
    gpio_clear(GPIOB, BRAKE4_PIN);

    /* PC14 */
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BRAKE5_PIN);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, BRAKE5_PIN);
    gpio_clear(GPIOC, BRAKE5_PIN);
}

/* PWM GPIO: конфігурація AF для TIM3 (PA6, PA7, PB0, PB1) і TIM1 (PA8, PA11).
 * RCC для таймерів вмикається тут; HWD_PWM_Init() конфігурує OC-канали пізніше. */
static void pwm_gpio_setup(void)
{
    /* TIM3: PA6=CH1, PA7=CH2 — AF2 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR0_PWM_GPIO_PIN | MOTOR1_PWM_GPIO_PIN);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            MOTOR0_PWM_GPIO_PIN | MOTOR1_PWM_GPIO_PIN);
    gpio_set_af(GPIOA, GPIO_AF2,
                MOTOR0_PWM_GPIO_PIN | MOTOR1_PWM_GPIO_PIN);

    /* TIM3: PB0=CH3, PB1=CH4 — AF2 */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR2_PWM_GPIO_PIN | MOTOR3_PWM_GPIO_PIN);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            MOTOR2_PWM_GPIO_PIN | MOTOR3_PWM_GPIO_PIN);
    gpio_set_af(GPIOB, GPIO_AF2,
                MOTOR2_PWM_GPIO_PIN | MOTOR3_PWM_GPIO_PIN);

    rcc_periph_clock_enable(RCC_TIM3);

    /* TIM1: PA8=CH1, PA11=CH4 — AF1 */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    MOTOR4_PWM_GPIO_PIN | MOTOR5_PWM_GPIO_PIN);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
                            MOTOR4_PWM_GPIO_PIN | MOTOR5_PWM_GPIO_PIN);
    gpio_set_af(GPIOA, GPIO_AF1,
                MOTOR4_PWM_GPIO_PIN | MOTOR5_PWM_GPIO_PIN);

    rcc_periph_clock_enable(RCC_TIM1);

    /* TIM1 — advanced timer: BDTR.MOE має бути встановлений до старту виходів.
     * Це можна зробити лише після ввімкнення RCC. HWD_PWM_Start() не робить це. */
    timer_enable_break_main_output(TIM1);
}

/* SPI2 master: PB13=SCK, PB14=MISO, PB15=MOSI (AF5), PB12=CS (GPIO SW NSS).
 * Mode 0 (CPOL=0, CPHA=0), 6.25 MHz (APB1 50 MHz / 8). */
static void spi2_master_setup(void)
{
    rcc_periph_clock_enable(EHUB_SPI_RCC);

    /* SCK, MISO, MOSI → AF5 */
    gpio_mode_setup(EHUB_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                    EHUB_SPI_SCK_PIN | EHUB_SPI_MISO_PIN | EHUB_SPI_MOSI_PIN);
    gpio_set_output_options(EHUB_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            EHUB_SPI_SCK_PIN | EHUB_SPI_MISO_PIN | EHUB_SPI_MOSI_PIN);
    gpio_set_af(EHUB_SPI_GPIO_PORT, EHUB_SPI_GPIO_AF,
                EHUB_SPI_SCK_PIN | EHUB_SPI_MISO_PIN | EHUB_SPI_MOSI_PIN);

    /* CS — software GPIO OUT, inactive HIGH */
    gpio_mode_setup(EHUB_SPI_NSS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    EHUB_SPI_NSS_PIN);
    gpio_set_output_options(EHUB_SPI_NSS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            EHUB_SPI_NSS_PIN);
    gpio_set(EHUB_SPI_NSS_PORT, EHUB_SPI_NSS_PIN);

    spi_init_master(EHUB_SPI,
                    EHUB_SPI_BAUDRATE,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1,
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);

    /* Software slave management: SSI=1 (master не отримує MODF fault) */
    spi_enable_software_slave_management(EHUB_SPI);
    spi_set_nss_high(EHUB_SPI);

    spi_enable(EHUB_SPI);
}

/* E-STOP: PC15 — вхід з підтяжкою, EXTI15 falling edge, highest priority. */
static void estop_setup(void)
{
    gpio_mode_setup(ESTOP_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, ESTOP_PIN);

    exti_select_source(ESTOP_EXTI, ESTOP_GPIO_PORT);
    exti_set_trigger(ESTOP_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(ESTOP_EXTI);

    nvic_set_priority(ESTOP_EXTI_IRQ, 0);  /* найвищий пріоритет */
    nvic_enable_irq(ESTOP_EXTI_IRQ);
}

#if defined(USE_HWD_UART) && !defined(USE_COMM_ASYNC)
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
#endif /* USE_HWD_UART && !USE_COMM_ASYNC */

/* TIM5 (32-bit): free-running 1 MHz лічильник для HWD_Timer_GetMicros(). */
static void micros_timer_setup(void)
{
    rcc_periph_clock_enable(MICROS_TIMER_RCC);

    timer_set_mode(MICROS_TIMER,
                   TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE,
                   TIM_CR1_DIR_UP);

    timer_set_prescaler(MICROS_TIMER, MICROS_TIMER_PRESCALER);
    timer_set_period(MICROS_TIMER, 0xFFFFFFFFU);

    /* Примусово завантажити PSC у активний регістр негайно */
    TIM_EGR(MICROS_TIMER) = TIM_EGR_UG;

    timer_enable_counter(MICROS_TIMER);
}

#ifndef USE_FREERTOS
static void systick_setup(void)
{
    systick_set_frequency(SYSTICK_FREQ, SYSTEM_CORE_CLOCK);
    systick_counter_enable();
    systick_interrupt_enable();
}
#endif

/* ISRs -----------------------------------------------------------------------*/

/* E-STOP: EXTI15 falling edge — апаратна кнопка аварійного зупину.
 * ISR лише встановлює прапорець; App має викликати Servo_EmergencyStop() у main loop. */
void exti15_10_isr(void)
{
    if (exti_get_flag_status(ESTOP_EXTI)) {
        exti_reset_request(ESTOP_EXTI);
        g_estop_active = true;
    }
}

/* Public API -----------------------------------------------------------------*/

Servo_Status_t Board_Init(void)
{
    clock_setup();
    gpio_rcc_setup();

    /* Гальма — fail-safe LOW до будь-якої App-ініціалізації */
    brake_gpio_setup();

    led_gpio_setup();
    pwm_gpio_setup();   /* вмикає RCC_TIM3, RCC_TIM1, встановлює BDTR.MOE для TIM1 */
    spi2_master_setup();
    estop_setup();
    micros_timer_setup();

#ifndef USE_FREERTOS
    systick_setup();
#endif

#if defined(USE_HWD_UART) && !defined(USE_COMM_ASYNC)
    uart_setup();
#endif

    return SERVO_OK;
}
