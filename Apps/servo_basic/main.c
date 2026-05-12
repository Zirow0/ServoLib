#include "board_config.h"
#include "ctrl/cascade.h"
#include "drv/motor/pwm.h"
#include "drv/position/incremental_encoder.h"
#include "drv/brake/gpio_brake.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"

#include "comm/servo_comm.h"
#include "hwd_uart_async.h"

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
static PWM_Motor_Driver_t           motor;
static HWD_PWM_Handle_t             pwm_fwd;
static Incremental_Encoder_Driver_t encoder;
static GPIO_Brake_Driver_t          brake;
static ACS712_Driver_t              current_driver;
static HWD_ADC_Handle_t             current_adc;
static Cascade_Controller_t         cascade;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

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
        .variant                 = ACS712_30A,
        .adc                     = &current_adc,
        .divider_ratio           = 0.65f,
        .overcurrent_threshold_a = 4.0f,
        .ema_alpha               = 0.5f,
    };
    ACS712_Create(&current_driver, &acs_cfg);
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
        .max_power        = 100.0f,
        .min_power        = 5.0f,
        .invert_direction = false,
    };
    Motor_Init(&motor.interface, &mot_params);

    /* ── Інкрементальний енкодер ─────────────────────────────────────────── */
    static const Incremental_Encoder_HW_t enc_hw = {
        .gpio_port_a = ENCODER_GPIO_PORT_A,
        .gpio_pin_a  = ENCODER_GPIO_PIN_A,
        .gpio_af_a   = ENCODER_GPIO_AF,
        .gpio_port_b = ENCODER_GPIO_PORT_B,
        .gpio_pin_b  = ENCODER_GPIO_PIN_B,
        .timer_base  = ENCODER_TIMER_BASE,
        .timer_rcc   = ENCODER_TIMER_RCC,
        .ic_channel  = 0U,
    };
    Incremental_Encoder_Create(&encoder, ENCODER_CPR, &enc_hw);
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
    Brake_Release(&brake.interface);

    /* ── Каскадний PID ───────────────────────────────────────────────────── */
    static const Cascade_Config_t casc_cfg = {
        .pos = {
            .kp      = 5.0f,
            .ki      = 0.0f,
            .kd      = 0.3f,
            .out_min = -10.0f,
            .out_max =  10.0f,
            .i_limit =  5.0f,
        },
        .vel = {
            .kp      = 0.5f,
            .ki      = 5.0f,
            .kd      = 0.0f,
            .out_min = -3.0f,
            .out_max =  3.0f,
            .i_limit =  2.0f,
        },
        .trq = {
            .kp      = 0.0f,
            .ki      = 5.0f,
            .kd      = 0.0f,
            .out_min = -100.0f,
            .out_max =  100.0f,
            .i_limit =  30.0f,
        },
        .ff_j      = 10.0f,
        .ff_b      = 0.0f,
        .slew_rate = 2000.0f,
    };
    Cascade_Init(&cascade, &casc_cfg, CASCADE_MODE_TRQ);
    cascade.target_current = 0.4f;

    while (1) {
        /* ── RX: декодування та прийом команд (тільки тут — CRC32 safe) ── */
        servo_comm_process_rx();

        servo_command_t cmd;
        if (servo_comm_get_command(&cmd)) {
            Cascade_SetMode(&cascade, (Cascade_Mode_t)cmd.mode);
            switch ((Cascade_Mode_t)cmd.mode) {
                case CASCADE_MODE_POS: cascade.target_pos     = cmd.target; break;
                case CASCADE_MODE_VEL: cascade.target_vel     = cmd.target; break;
                case CASCADE_MODE_TRQ: cascade.target_current = cmd.target; break;
                default: break;
            }
        }

        /* ── Оновлення датчиків ──────────────────────────────────────────── */
        Current_Sensor_Update(&current_driver.interface);
        Brake_Update(&brake.interface);
        Position_Sensor_Update(&encoder.interface);

        float pos_rad   = 0.0f;
        float vel_rad_s = 0.0f;
        float current_a = 0.0f;

        Position_Sensor_GetPosition(&encoder.interface, &pos_rad);
        Position_Sensor_GetVelocity(&encoder.interface, &vel_rad_s);
        Current_Sensor_GetCurrent(&current_driver.interface, &current_a);

        /* ── Каскадний PID → команда двигуну ────────────────────────────── */
        uint32_t now_us = HWD_Timer_GetMicros();
        float power = Cascade_Compute(&cascade, pos_rad, vel_rad_s, current_a, now_us);
        Motor_SetPower(&motor.interface, power);

        /* ── TX: телеметрія 100 Гц ───────────────────────────────────────── */
        static uint32_t last_telem = 0;
        uint32_t now_ms = HWD_Timer_GetMillis();
        if (now_ms - last_telem >= 10U) {
            last_telem = now_ms;

            float target = 0.0f;
            switch (cascade.mode) {
                case CASCADE_MODE_POS: target = cascade.target_pos;     break;
                case CASCADE_MODE_VEL: target = cascade.target_vel;     break;
                case CASCADE_MODE_TRQ: target = cascade.target_current; break;
                default: break;
            }

            servo_telemetry_t telem = {
                .position_rad   = pos_rad,
                .velocity_rad_s = vel_rad_s,
                .current_a      = current_a,
                .target         = target,
                .mode           = (uint8_t)cascade.mode,
                .timestamp_ms   = now_ms,
            };
            servo_comm_send_telemetry(&telem);

            HWD_GPIO_TogglePin(&led_pin);
        }

        HWD_Timer_DelayMs(1);
    }
}
