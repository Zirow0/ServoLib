/**
 * @file incremental_encoder.c
 * @brief Реалізація драйвера інкрементального квадратурного енкодера
 *
 * Позиція: EXTI X4 state machine → volatile int32_t count
 * Швидкість: TIM Input Capture → volatile uint32_t period_us
 *
 * Dispatch tables дозволяють реєструвати до ENC_MAX (6) енкодерів:
 *   s_exti_map[16]  — індекс = номер EXTI лінії (0..15)
 *   s_tim_map[]     — лінійний пошук за timer_base
 *
 * EXTI 5-9  → exti9_5_isr  (group dispatch по всіх активних лініях)
 * EXTI 10-15 → exti15_10_isr
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_INCREMENTAL

#include "../../../Inc/drv/position/incremental_encoder.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <libopencm3/stm32/exti.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/** Таймаут відсутності імпульсів → швидкість = 0 */
#define ENC_SPEED_TIMEOUT_MS  500U

/** Максимальна кількість одночасно активних енкодерів */
#define ENC_MAX               6U

/* Private types -------------------------------------------------------------*/

typedef struct {
    uint32_t                      timer_base;
    uint32_t                      ic_ch;        /**< 1..4 */
    volatile uint32_t            *ccr_reg;      /**< вказівник на TIM_CCRx, вирахований в HW_Init */
    uint32_t                      last_ccr;
    Incremental_Encoder_Driver_t *driver;
} EncTimEntry_t;

/* Private data --------------------------------------------------------------*/

/**
 * X4 state machine lookup table.
 * Індекс = (старий_стан << 2) | новий_стан, де стан = (B << 1) | A
 *  0 → немає руху, +1 → вперед, -1 → назад
 */
static const int8_t enc_table[16] = {
/*  нов: 00   01   10   11    стар */
          0,  -1,  +1,   0,  /* 00 */
         +1,   0,   0,  -1,  /* 01 */
         -1,   0,   0,  +1,  /* 10 */
          0,  +1,  -1,   0   /* 11 */
};

/* Dispatch tables */
static Incremental_Encoder_Driver_t *s_exti_map[16];   /* indexed by EXTI line 0..15 */
static EncTimEntry_t                 s_tim_map[ENC_MAX];
static uint8_t                       s_tim_count = 0U;

/* NVIC IRQ lookup for EXTI lines 0..15 */
static const uint8_t s_exti_nvic[16] = {
    NVIC_EXTI0_IRQ,     /* 0  */
    NVIC_EXTI1_IRQ,     /* 1  */
    NVIC_EXTI2_IRQ,     /* 2  */
    NVIC_EXTI3_IRQ,     /* 3  */
    NVIC_EXTI4_IRQ,     /* 4  */
    NVIC_EXTI9_5_IRQ,   /* 5  */
    NVIC_EXTI9_5_IRQ,   /* 6  */
    NVIC_EXTI9_5_IRQ,   /* 7  */
    NVIC_EXTI9_5_IRQ,   /* 8  */
    NVIC_EXTI9_5_IRQ,   /* 9  */
    NVIC_EXTI15_10_IRQ, /* 10 */
    NVIC_EXTI15_10_IRQ, /* 11 */
    NVIC_EXTI15_10_IRQ, /* 12 */
    NVIC_EXTI15_10_IRQ, /* 13 */
    NVIC_EXTI15_10_IRQ, /* 14 */
    NVIC_EXTI15_10_IRQ, /* 15 */
};

/* Private functions ---------------------------------------------------------*/

static uint8_t get_timer_nvic(uint32_t timer_base)
{
    if (timer_base == TIM2) return NVIC_TIM2_IRQ;
    if (timer_base == TIM3) return NVIC_TIM3_IRQ;
    if (timer_base == TIM4) return NVIC_TIM4_IRQ;
    return 0xFFU;
}

/* Private dispatch helpers --------------------------------------------------*/

static void dispatch_exti(uint8_t line)
{
    Incremental_Encoder_Driver_t *d = s_exti_map[line];
    if (d == NULL) {
        exti_reset_request(1U << line);
        return;
    }

    /* Атомарне читання IDR ПЕРЕД очисткою прапора — мінімальна затримка */
    uint32_t idr_a = GPIO_IDR(d->hw.gpio_port_a);
    uint32_t idr_b = (d->hw.gpio_port_b == d->hw.gpio_port_a)
                     ? idr_a : GPIO_IDR(d->hw.gpio_port_b);
    exti_reset_request(1U << line);

    uint8_t a = (idr_a & d->hw.gpio_pin_a) ? 1U : 0U;
    uint8_t b = (idr_b & d->hw.gpio_pin_b) ? 1U : 0U;
    Incremental_Encoder_EXTI_Handler(d, a, b);
}

static void dispatch_tim_ic(uint32_t timer_base)
{
    for (uint8_t i = 0U; i < s_tim_count; i++) {
        EncTimEntry_t *e = &s_tim_map[i];
        if (e->timer_base != timer_base) { continue; }

        /* TIM_SR_CC1IF = (1<<1); для CHx (0-based): (1<<1) << ic_ch */
        uint32_t flag = (1U << 1U) << e->ic_ch;
        if (!timer_get_flag(timer_base, flag)) { continue; }
        timer_clear_flag(timer_base, flag);

        uint32_t ccr    = *e->ccr_reg;
        /* uint16_t cast: коректний wrap для 16-bit таймерів (TIM3/TIM4);
         * для 32-bit (TIM2) обмежує max вимірюваний період до ~65 мс,
         * що відповідає ~2 RPM при CPR=4000 — прийнятно для servo */
        uint32_t period = (uint32_t)(uint16_t)(ccr - e->last_ccr);
        e->last_ccr     = ccr;
        if (e->driver != NULL && period > 0U) {
            Incremental_Encoder_IC_Handler(e->driver, period);
        }
    }
}

/* Private hardware callbacks ------------------------------------------------*/

static Servo_Status_t IncEnc_HW_Init(void *driver_data)
{
    Incremental_Encoder_Driver_t *drv = (Incremental_Encoder_Driver_t *)driver_data;
    const Incremental_Encoder_HW_t *hw = &drv->hw;

    drv->count         = 0;
    drv->period_us     = 0U;
    drv->last_pulse_ms = 0U;
    drv->direction     = 1;

    /* ── Реєстрація в dispatch tables ── */
    uint8_t line_a = (uint8_t)__builtin_ctz(hw->gpio_pin_a);
    uint8_t line_b = (uint8_t)__builtin_ctz(hw->gpio_pin_b);
    s_exti_map[line_a] = drv;
    s_exti_map[line_b] = drv;

    if (s_tim_count < ENC_MAX) {
        /* Вирахувати ccr_reg тут — єдине місце де TIM_CCRx потрібні */
        /* ic_channel 0-based: 0=CH1, 1=CH2, 2=CH3, 3=CH4 (відповідає enum tim_ic_id) */
        volatile uint32_t *ccr;
        switch (hw->ic_channel) {
            case 0U: ccr = &TIM_CCR1(hw->timer_base); break;
            case 1U: ccr = &TIM_CCR2(hw->timer_base); break;
            case 2U: ccr = &TIM_CCR3(hw->timer_base); break;
            default:  ccr = &TIM_CCR4(hw->timer_base); break;
        }
        s_tim_map[s_tim_count].timer_base = hw->timer_base;
        s_tim_map[s_tim_count].ic_ch      = hw->ic_channel;
        s_tim_map[s_tim_count].ccr_reg    = ccr;
        s_tim_map[s_tim_count].last_ccr   = 0U;
        s_tim_map[s_tim_count].driver     = drv;
        s_tim_count++;
    }

    /* ── Тактування ── */
    rcc_periph_clock_enable(RCC_SYSCFG);
    rcc_periph_clock_enable(hw->timer_rcc);

    /* ── Пін A → AF (TIM IC) + pull-up ── */
    gpio_mode_setup(hw->gpio_port_a, GPIO_MODE_AF, GPIO_PUPD_PULLUP, hw->gpio_pin_a);
    gpio_set_af(hw->gpio_port_a, hw->gpio_af_a, hw->gpio_pin_a);

    /* ── Пін B → GPIO input + pull-up ── */
    gpio_mode_setup(hw->gpio_port_b, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, hw->gpio_pin_b);

    /* ── TIM Input Capture ── */
    /* Prescaler: APB1_TIMER_CLOCK / 1 MHz → 1 мкс/тік */
    timer_set_mode(hw->timer_base, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(hw->timer_base, APB1_TIMER_CLOCK / 1000000U - 1U);
    timer_set_period(hw->timer_base, 0xFFFFFFFFU);

    /* ic_channel 0-based → enum tim_ic_id напряму (TIM_IC1=0, TIM_IC2=1, ...) */
    /* TIM_IC_IN_TIx: TI1=1, TI2=2, TI3=5, TI4=6 — потрібен lookup */
    static const enum tim_ic_input ic_in_map[4] = {
        TIM_IC_IN_TI1, TIM_IC_IN_TI2, TIM_IC_IN_TI3, TIM_IC_IN_TI4
    };
    enum tim_ic_id    ic_id = (enum tim_ic_id)hw->ic_channel;
    enum tim_ic_input ic_in = ic_in_map[hw->ic_channel < 4U ? hw->ic_channel : 0U];

    timer_ic_set_input(hw->timer_base, ic_id, ic_in);
    timer_ic_set_filter(hw->timer_base, ic_id, TIM_IC_OFF);
    timer_ic_set_prescaler(hw->timer_base, ic_id, TIM_IC_PSC_OFF);
    timer_ic_set_polarity(hw->timer_base, ic_id, TIM_IC_RISING);
    timer_ic_enable(hw->timer_base, ic_id);

    /* CC IRQ: TIM_DIER_CC1IE = (1<<1); для CHx (0-based): зсув на ic_ch */
    timer_enable_irq(hw->timer_base, (uint32_t)((1U << 1U) << hw->ic_channel));

    uint8_t tim_nvic = get_timer_nvic(hw->timer_base);
    if (tim_nvic != 0xFFU) {
        nvic_set_priority(tim_nvic, 1U);
        nvic_enable_irq(tim_nvic);
    }

    timer_enable_counter(hw->timer_base);

    /* ── Ініціалізація enc_state з фактичного стану пінів ── */
    uint32_t idr_a = GPIO_IDR(hw->gpio_port_a);
    uint32_t idr_b = (hw->gpio_port_b == hw->gpio_port_a)
                     ? idr_a : GPIO_IDR(hw->gpio_port_b);
    uint8_t init_a = (idr_a & hw->gpio_pin_a) ? 1U : 0U;
    uint8_t init_b = (idr_b & hw->gpio_pin_b) ? 1U : 0U;
    drv->enc_state = (uint8_t)((init_b << 1) | init_a);

    /* ── EXTI для каналу A (обидва фронти) ── */
    exti_select_source(1U << line_a, hw->gpio_port_a);
    exti_set_trigger(1U << line_a, EXTI_TRIGGER_BOTH);
    exti_enable_request(1U << line_a);
    nvic_set_priority(s_exti_nvic[line_a], 0U);
    nvic_enable_irq(s_exti_nvic[line_a]);

    /* ── EXTI для каналу B (обидва фронти) ── */
    exti_select_source(1U << line_b, hw->gpio_port_b);
    exti_set_trigger(1U << line_b, EXTI_TRIGGER_BOTH);
    exti_enable_request(1U << line_b);
    nvic_set_priority(s_exti_nvic[line_b], 0U);
    nvic_enable_irq(s_exti_nvic[line_b]);

    return SERVO_OK;
}

static Servo_Status_t IncEnc_HW_ReadRaw(void *driver_data, Position_Raw_Data_t *raw)
{
    Incremental_Encoder_Driver_t *drv = (Incremental_Encoder_Driver_t *)driver_data;
    const int32_t cpr = (int32_t)drv->cpr;

    /* Читання volatile — атомарне для 32-bit на Cortex-M4 */
    int32_t  count     = drv->count;
    uint32_t period_us = drv->period_us;
    uint32_t last_ms   = drv->last_pulse_ms;
    int8_t   dir       = drv->direction;

    /* Абсолютний кут напряму з лічильника ISR — multi-turn вбудований */
    raw->angle_rad    = (float)count / (float)cpr * TWO_PI;
    raw->timestamp_us = HWD_Timer_GetMicros();
    raw->valid        = true;

    /* Швидкість з IC timer */
    if (period_us == 0U || (HWD_Timer_GetMillis() - last_ms) > ENC_SPEED_TIMEOUT_MS) {
        raw->has_velocity   = true;
        raw->velocity_rad_s = 0.0f;
    } else {
        /* один тік каналу A = 1/CPR оберту → TWO_PI/CPR радіанів */
        float vel = TWO_PI / ((float)period_us * 1e-6f * (float)drv->cpr);
        raw->has_velocity   = true;
        raw->velocity_rad_s = (dir >= 0) ? vel : -vel;
    }

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t *driver,
                                           uint32_t cpr,
                                           const Incremental_Encoder_HW_t *hw)
{
    if (driver == NULL || cpr == 0U || hw == NULL) {
        return SERVO_INVALID;
    }

    memset(driver, 0, sizeof(Incremental_Encoder_Driver_t));

    driver->cpr = cpr;
    driver->hw  = *hw;

    driver->interface.hw.init     = IncEnc_HW_Init;
    driver->interface.hw.read_raw = IncEnc_HW_ReadRaw;
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

void Incremental_Encoder_EXTI_Handler(Incremental_Encoder_Driver_t *driver,
                                       uint8_t pin_a, uint8_t pin_b)
{
    uint8_t curr = (uint8_t)((pin_b << 1) | pin_a);
    uint8_t idx  = (uint8_t)((driver->enc_state << 2) | curr);
    int8_t  step = enc_table[idx];

    driver->count    += step;
    driver->enc_state = curr;

    if (step != 0) {
        driver->direction = step;
    }
}

void Incremental_Encoder_IC_Handler(Incremental_Encoder_Driver_t *driver,
                                     uint32_t period_us)
{
    driver->period_us     = period_us;
    driver->last_pulse_ms = HWD_Timer_GetMillis();
}

/* ISR handlers --------------------------------------------------------------*/

void exti0_isr(void)      { dispatch_exti(0U); }
void exti1_isr(void)      { dispatch_exti(1U); }
void exti2_isr(void)      { dispatch_exti(2U); }
void exti3_isr(void)      { dispatch_exti(3U); }
void exti4_isr(void)      { dispatch_exti(4U); }

void exti9_5_isr(void)
{
    for (uint8_t l = 5U; l <= 9U; l++) {
        if (exti_get_flag_status(1U << l)) { dispatch_exti(l); }
    }
}

void exti15_10_isr(void)
{
    for (uint8_t l = 10U; l <= 15U; l++) {
        if (exti_get_flag_status(1U << l)) { dispatch_exti(l); }
    }
}

void tim2_isr(void) { dispatch_tim_ic(TIM2); }
void tim3_isr(void) { dispatch_tim_ic(TIM3); }
void tim4_isr(void) { dispatch_tim_ic(TIM4); }

#endif /* USE_SENSOR_INCREMENTAL */
