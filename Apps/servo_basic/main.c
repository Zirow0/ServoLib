#include "board_config.h"
#include "ctrl/cascade.h"
#include "ctrl/time.h"
#include "drv/motor/pwm.h"
#include "drv/position/incremental_encoder.h"
#include "drv/brake/gpio_brake.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_gpio.h"

#include "comm/servo_comm.h"
#include "hwd_uart_async.h"

#include <math.h>

/* ================================================================
 * UART async екземпляр
 * USART1: PA9 TX, PA10 RX
 * RX DMA2 Stream2 Ch4, TX DMA2 Stream7 Ch4
 * ================================================================ */
static const uart_hw_config_t comm_hw = {
    .usart      = COMM_USART,
    .tx_port    = COMM_GPIO_PORT,  .tx_pin    = COMM_TX_PIN,
    .rx_port    = COMM_GPIO_PORT,  .rx_pin    = COMM_RX_PIN,
    .rx_dma     = COMM_RX_DMA,
    .rx_stream  = COMM_RX_DMA_STREAM,
    .rx_channel = COMM_RX_DMA_CHANNEL,
    .tx_dma     = COMM_TX_DMA,
    .tx_stream  = COMM_TX_DMA_STREAM,
    .tx_channel = COMM_TX_DMA_CHANNEL,
};

static uart_instance_t comm_inst;

static bool comm_send(const uint8_t *data, size_t len)
{
    return uart_send(&comm_inst, data, len);
}

/* ================================================================
 * Апаратні об'єкти
 * ================================================================ */
#define STOP_VEL_THRESHOLD_RAD_S  0.05f

typedef enum { APP_RUNNING, APP_STOPPING, APP_STOPPED, APP_ESTOP } App_State_t;

static PWM_Motor_Driver_t           motor;
static HWD_PWM_Handle_t             pwm_fwd;
static Incremental_Encoder_Driver_t encoder;
static GPIO_Brake_Driver_t          brake;
static ACS712_Driver_t              current_driver;
static HWD_ADC_Handle_t             current_adc;
static Cascade_Controller_t         cascade;
static App_State_t                  app_state = APP_STOPPED;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

/* ================================================================
 * Спільний стан між callback-ами
 * ================================================================ */
static float s_pos_rad      = 0.0f;
static float s_vel_rad_s    = 0.0f;
static float s_accel_rad_s2 = 0.0f;
static float s_current_a    = 0.0f;

/* ================================================================
 * Діагностика частоти (лічильники скидаються раз на секунду)
 * ================================================================ */
static volatile uint32_t s_sensor_count  = 0;
static volatile uint32_t s_control_count = 0;
static volatile uint32_t s_telem_count   = 0;

/* Фактичний крок часу між виконаннями on_control (мкс) — для перевірки 1 кГц */
static volatile uint32_t s_control_dt_us = 0;
/* Фактичний крок часу між виконаннями on_sensor (мкс) — для перевірки 10 кГц */
static volatile uint32_t s_sensor_dt_us  = 0;

/* ================================================================
 * Програмні таймери
 * ================================================================ */
static Cb_Timer_t sensor_timer;   /* 10 кГц */
static Cb_Timer_t control_timer;  /* 1 кГц  */
static Cb_Timer_t telem_timer;    /* 100 Гц */
static Cb_Timer_t diag_timer;     /* 1 Гц  — скидає лічильники, верифікує Hz */

/* ================================================================
 * Callback-и таймерів
 * ================================================================ */
static void on_sensor(void)
{
    static uint32_t last_us = 0;
    uint32_t now = Time_GetMicros();
    if (last_us != 0U) { s_sensor_dt_us = now - last_us; }
    last_us = now;
    s_sensor_count++;

    /* Лише струм — 1-стан UKF, ~5–10 мкс */
    Current_Sensor_Update(&current_driver.interface, 0.0001f);
    Current_Sensor_GetCurrent(&current_driver.interface, &s_current_a);
}

static void on_control(void)
{
    static uint32_t last_us = 0;
    uint32_t now = Time_GetMicros();
    if (last_us != 0U) { s_control_dt_us = now - last_us; }
    last_us = now;
    s_control_count++;

    /* Енкодер UKF (3-стан) — ~100–300 мкс, достатньо бюджету при 1 кГц */
    Position_Sensor_Update(&encoder.interface);
    Position_Sensor_GetPosition(&encoder.interface,     &s_pos_rad);
    Position_Sensor_GetVelocity(&encoder.interface,     &s_vel_rad_s);
    Position_Sensor_GetAcceleration(&encoder.interface, &s_accel_rad_s2);

    Brake_Update(&brake.interface);

    if (app_state == APP_STOPPING &&
        fabsf(s_vel_rad_s) < STOP_VEL_THRESHOLD_RAD_S)
    {
        Motor_Stop(&motor.interface);
        Brake_Engage(&brake.interface);
        Cascade_Reset(&cascade);
        app_state = APP_STOPPED;
    }

    if (app_state == APP_RUNNING || app_state == APP_STOPPING) {
        float power = Cascade_Compute(&cascade, s_pos_rad, s_vel_rad_s,
                                      s_current_a, now);
        Motor_SetPower(&motor.interface, power);
    }
}

static void on_telem(void)
{
    s_telem_count++;

    float target = 0.0f;
    switch (cascade.mode) {
        case CASCADE_MODE_POS: target = cascade.target_pos;     break;
        case CASCADE_MODE_VEL: target = cascade.target_vel;     break;
        case CASCADE_MODE_TRQ: target = cascade.target_current; break;
        default: break;
    }

    cascade_telemetry_t telem = {
        .timestamp_ms   = Time_GetMillis(),
        .position_rad   = s_pos_rad,
        .velocity_rad_s = s_vel_rad_s,
        .accel_rad_s2   = s_accel_rad_s2,
        .current_a      = s_current_a,
        .target         = target,
        .vel_sp         = cascade.last_vel_sp,
        .current_sp     = cascade.last_current_sp,
        .ff             = cascade.last_ff,
        .power          = cascade.last_power,
        .pos_p          = cascade.pos_pid.p_term,
        .pos_i          = cascade.pos_pid.i_term,
        .pos_d          = cascade.pos_pid.d_term,
        .vel_p          = cascade.vel_pid.p_term,
        .vel_i          = cascade.vel_pid.i_term,
        .vel_d          = cascade.vel_pid.d_term,
        .vel_integral   = cascade.vel_pid.integral,
        .trq_p          = cascade.trq_pid.p_term,
        .trq_i          = cascade.trq_pid.i_term,
        .trq_d          = cascade.trq_pid.d_term,
        .trq_integral   = cascade.trq_pid.integral,
        .mode           = (uint8_t)cascade.mode,
    };
    servo_comm_send_cascade(&telem);

    HWD_GPIO_TogglePin(&led_pin);
}

/* Раз на секунду — читає лічильники і перевіряє відповідність частот.
 * Фактичні значення зберігаються у статичних змінних нижче і доступні
 * через ST-Link live watch для верифікації без зміни протоколу. */
static volatile uint32_t s_diag_sensor_hz  = 0;  /* фактична частота on_sensor, Гц */
static volatile uint32_t s_diag_control_hz = 0;  /* фактична частота on_control, Гц */
static volatile uint32_t s_diag_telem_hz   = 0;  /* фактична частота on_telem, Гц */

static void on_diag(void)
{
    s_diag_sensor_hz  = s_sensor_count;
    s_diag_control_hz = s_control_count;
    s_diag_telem_hz   = s_telem_count;
    s_sensor_count    = 0;
    s_control_count   = 0;
    s_telem_count     = 0;
}

/* ================================================================
 * Main
 * ================================================================ */
int main(void)
{
    Board_Init();
    frame_crc32_init();

    /* ── Comm ────────────────────────────────────────────────────────────── */
    servo_comm_init(comm_send);
    uart_init(&comm_inst, &comm_hw, COMM_BAUD, servo_comm_on_rx);

    /* ── Датчик струму ACS712 ────────────────────────────────────────────── */
    static const HWD_ADC_Config_t adc_cfg = {
        .adc_base  = CURRENT_ADC_PERIPH,
        .rcc_adc   = CURRENT_ADC_RCC,
        .rcc_gpio  = CURRENT_ADC_GPIO_RCC,
        .gpio_port = CURRENT_ADC_GPIO_PORT,
        .gpio_pin  = CURRENT_ADC_GPIO_PIN,
        .channel   = CURRENT_ADC_CHANNEL,
        .vref_v    = CURRENT_ADC_VREF_V,
    };
    HWD_ADC_Init(&current_adc, &adc_cfg);
    HWD_ADC_StartScan();

    static const ACS712_Config_t acs_cfg = {
        .variant       = ACS712_30A,
        .adc           = &current_adc,
        .divider_ratio = 0.65f,
    };
    static const Current_Params_t current_params = {
        .overcurrent_threshold_a = 4.0f,
        .process_noise_q         = 0.0001f,
        .measurement_noise_r     = 0.5f,
    };
    ACS712_Create(&current_driver, &acs_cfg, &current_params);
    Time_DelayMs(1000U);
    Current_Sensor_Calibrate(&current_driver.interface);

    /* ── PWM канал ───────────────────────────────────────────────────────── */
    HWD_PWM_Config_t fwd_cfg = {
        .frequency  = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .hw_handle  = (void*)MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_OC_FWD,
    };
    HWD_PWM_Init(&pwm_fwd, &fwd_cfg);

    /* ── Двигун (PWM + DIR) ──────────────────────────────────────────────── */
    PWM_Motor_Config_t mot_cfg = {
        .type     = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,
        .pwm_fwd  = &pwm_fwd,
        .pwm_bwd  = NULL,
        .gpio_dir = (void*)MOTOR_DIR_GPIO_PORT,
        .gpio_pin = MOTOR_DIR_PIN,
    };
    PWM_Motor_Create(&motor, &mot_cfg);

    Motor_Params_t mot_params = {
        .max_power        = 99.9f,
        .min_power        = 1.0f,
        .invert_direction = false,
    };
    Motor_Init(&motor.interface, &mot_params);

    /* ── Інкрементальний енкодер ─────────────────────────────────────────── */
    static const Incremental_Encoder_HW_t enc_hw = {
        .gpio_port_a = ENC0_GPIO_PORT_A,
        .gpio_pin_a  = ENC0_GPIO_PIN_A,
        .gpio_af_a   = ENC0_GPIO_AF,
        .gpio_port_b = ENC0_GPIO_PORT_B,
        .gpio_pin_b  = ENC0_GPIO_PIN_B,
        .timer_base  = ENC0_TIMER_BASE,
        .timer_rcc   = ENC0_TIMER_RCC,
        .ic_channel  = 0U,
    };
    Incremental_Encoder_Create(&encoder, ENC0_CPR, &enc_hw);
    Position_Sensor_Init(&encoder.interface);

    /* ── Гальмо ──────────────────────────────────────────────────────────── */
    GPIO_Brake_Config_t brk_cfg = {
        .gpio_port       = (void*)BRAKE_CTRL_GPIO_PORT,
        .gpio_pin        = BRAKE_CTRL_PIN,
        .active_high     = false,
        .engage_time_ms  = 50,
        .release_time_ms = 30,
    };
    GPIO_Brake_Create(&brake, &brk_cfg);

    /* ── Каскадний PID ───────────────────────────────────────────────────── */
    static const Cascade_Config_t casc_cfg = {
        .pos = {
            .kp      = 6.5f,
            .ki      = 0.0f,
            .kd      = 0.1f,
            .out_min = -0.2f,
            .out_max =  0.2f,
            .i_limit =  0.1f,
        },
        .vel = {
            .kp      = 8.0f,
            .ki      = 73.0f,
            .kd      = 0.0f,
            .out_min = -4.0f,
            .out_max =  4.0f,
            .i_limit =  3.0f,
        },
        .trq = {
            .kp      = 0.0f,
            .ki      = 100.0f,
            .kd      = 0.0f,
            .out_min = -100.0f,
            .out_max =  100.0f,
            .i_limit =  40.0f,
        },
        .ff_r      = 11.15f, /* %/А — емпірично; R_eff = 11.15×24/100 = 2.68 Ом ≈ R_вим */
        .ff_bemf   = 120.0f, /* %·с/рад — емпірично; Ke = 120×24/100 = 28.8 В/(рад/с) */
        .slew_rate = 5000.0f,
    };
    Cascade_Init(&cascade, &casc_cfg, CASCADE_MODE_POS);
    cascade.target_pos = 0.0f;

    const cascade_config_t seed = {
        .pos_kp      = casc_cfg.pos.kp,      .pos_ki      = casc_cfg.pos.ki,
        .pos_kd      = casc_cfg.pos.kd,      .pos_out_min = casc_cfg.pos.out_min,
        .pos_out_max = casc_cfg.pos.out_max,  .pos_i_limit = casc_cfg.pos.i_limit,
        .vel_kp      = casc_cfg.vel.kp,      .vel_ki      = casc_cfg.vel.ki,
        .vel_kd      = casc_cfg.vel.kd,      .vel_out_min = casc_cfg.vel.out_min,
        .vel_out_max = casc_cfg.vel.out_max,  .vel_i_limit = casc_cfg.vel.i_limit,
        .trq_kp      = casc_cfg.trq.kp,      .trq_ki      = casc_cfg.trq.ki,
        .trq_kd      = casc_cfg.trq.kd,      .trq_out_min = casc_cfg.trq.out_min,
        .trq_out_max = casc_cfg.trq.out_max,  .trq_i_limit = casc_cfg.trq.i_limit,
        .ff_j        = casc_cfg.ff_r,
        .ff_b        = casc_cfg.ff_bemf,
        .slew_rate   = casc_cfg.slew_rate,
    };
    servo_comm_seed_cascade_config(&seed);

    /* ── Ініціалізація таймерів ──────────────────────────────────────────── */
    Time_CbTimerInit(&sensor_timer,  on_sensor,  200U);     /* 5 кГц   */
    Time_CbTimerInit(&control_timer, on_control, 1000U);     /* 1 кГц  */
    Time_CbTimerInit(&telem_timer,   on_telem,   10000U);    /* 100 Гц */
    Time_CbTimerInit(&diag_timer,    on_diag,    1000000U);  /* 1 Гц   */

    while (1) {
        /* ── RX: декодування та прийом команд (тільки тут — CRC32 safe) ── */
        servo_comm_process_rx();

        /* Аварійна зупинка: найвищий пріоритет */
        if (servo_comm_get_estop()) {
            Motor_EmergencyStop(&motor.interface);
            Brake_Engage(&brake.interface);
            Cascade_Reset(&cascade);
            app_state = APP_ESTOP;
        }

        /* Команда зупинки */
        if (app_state != APP_ESTOP && servo_comm_get_stop()) {
            Cascade_SetMode(&cascade, CASCADE_MODE_VEL);
            cascade.target_vel = 0.0f;
            app_state = APP_STOPPING;
        }

        /* Команда руху */
        servo_command_t cmd;
        if (app_state != APP_ESTOP && servo_comm_get_command(&cmd)) {
            if (app_state == APP_STOPPED) {
                Brake_Release(&brake.interface);
                app_state = APP_RUNNING;
            }
            Cascade_SetMode(&cascade, (Cascade_Mode_t)cmd.mode);
            switch ((Cascade_Mode_t)cmd.mode) {
                case CASCADE_MODE_POS: cascade.target_pos     = cmd.target; break;
                case CASCADE_MODE_VEL: cascade.target_vel     = cmd.target; break;
                case CASCADE_MODE_TRQ: cascade.target_current = cmd.target; break;
                default: break;
            }
        }

        /* Конфігурація PID online */
        cascade_config_t wire_cfg;
        if (servo_comm_get_cascade_config(&wire_cfg)) {
            const Cascade_Config_t new_cfg = {
                .pos = {
                    .kp      = wire_cfg.pos_kp,
                    .ki      = wire_cfg.pos_ki,
                    .kd      = wire_cfg.pos_kd,
                    .out_min = wire_cfg.pos_out_min,
                    .out_max = wire_cfg.pos_out_max,
                    .i_limit = wire_cfg.pos_i_limit,
                },
                .vel = {
                    .kp      = wire_cfg.vel_kp,
                    .ki      = wire_cfg.vel_ki,
                    .kd      = wire_cfg.vel_kd,
                    .out_min = wire_cfg.vel_out_min,
                    .out_max = wire_cfg.vel_out_max,
                    .i_limit = wire_cfg.vel_i_limit,
                },
                .trq = {
                    .kp      = wire_cfg.trq_kp,
                    .ki      = wire_cfg.trq_ki,
                    .kd      = wire_cfg.trq_kd,
                    .out_min = wire_cfg.trq_out_min,
                    .out_max = wire_cfg.trq_out_max,
                    .i_limit = wire_cfg.trq_i_limit,
                },
                .ff_r      = wire_cfg.ff_j,
                .ff_bemf   = wire_cfg.ff_b,
                .slew_rate = wire_cfg.slew_rate,
            };
            Cascade_ApplyConfig(&cascade, &new_cfg);
            servo_comm_send_cascade_config(&wire_cfg);
        }

        /* ── Виклик таймерів ─────────────────────────────────────────────── */
        Time_CbTimerTick(&sensor_timer);
        Time_CbTimerTick(&control_timer);
        Time_CbTimerTick(&telem_timer);
        Time_CbTimerTick(&diag_timer);
    }
}
