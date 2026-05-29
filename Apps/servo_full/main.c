#include "board_config.h"
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/position/incremental_encoder.h"
#include "drv/brake/gpio_brake.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>

static PWM_Motor_Driver_t           motor;
static HWD_PWM_Handle_t             pwm_fwd;
static Incremental_Encoder_Driver_t encoder;
static GPIO_Brake_Driver_t          brake;
static ACS712_Driver_t              current_driver;
static HWD_ADC_Handle_t             current_adc;
static Servo_Controller_t           servo;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

int main(void)
{
    Board_Init();

    HWD_UART_WriteString("ServoLib servo_full debug\r\n");

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
        .process_noise_q         = 0.001f,
        .measurement_noise_r     = 0.02f,
    };
    ACS712_Create(&current_driver, &acs_cfg, &current_params);
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
        .gpio_port_a = ENC0_GPIO_PORT_A,
        .gpio_pin_a  = ENC0_GPIO_PIN_A,
        .gpio_af_a   = ENC0_GPIO_AF,
        .gpio_port_b = ENC0_GPIO_PORT_B,
        .gpio_pin_b  = ENC0_GPIO_PIN_B,
        .timer_base  = ENC0_TIMER_BASE,
        .timer_rcc   = ENC0_TIMER_RCC,
        .ic_channel  = 0U,  /* TIM_IC1 = CH1 (0-based, відповідає enum tim_ic_id) */
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

    /* ── Сервоконтролер ──────────────────────────────────────────────────── */
    Servo_Config_t srv_cfg = {
        .update_frequency = 1000.0f,
        .enable_brake     = true,

        .cascade_config = {
            /* pos-контур: положення → setpoint швидкості (рад/с) */
            .pos = {
                .kp      = 5.0f,
                .ki      = 0.0f,
                .kd      = 0.3f,
                .out_min = -10.0f,   /* рад/с */
                .out_max =  10.0f,
                .i_limit =  5.0f,
            },
            /* vel-контур: швидкість → setpoint струму (А) */
            .vel = {
                .kp      = 0.5f,
                .ki      = 5.0f,
                .kd      = 0.0f,
                .out_min = -3.0f,    /* А — робочий ліміт струму */
                .out_max =  3.0f,
                .i_limit =  2.0f,
            },
            /* trq-контур: струм → команда двигуну (%) */
            .trq = {
                .kp      = 0.0f,
                .ki      = 0.0f,
                .kd      = 0.0f,
                .out_min = -100.0f,  /* % */
                .out_max =  100.0f,
                .i_limit =  30.0f,
            },
            .ff_j      = 0.0f,    /* А/(рад/с²) — потребує тюнінгу */
            .ff_b      = 0.0f,    /* А·с/рад */
            .slew_rate = 5000.0f,    /* %/с, 0 = вимкнено */
        },

        .safety_config = {
            .min_position           = 0.0f,          /* рад */
            .max_position           = 6.2832f,        /* 2π рад = 360° */
            .enable_position_limits = true,

            .max_velocity           = 60.2832f,        /* рад/с ≈ 360°/с */
            .enable_velocity_limit  = true,

            .max_acceleration       = 120.5664f,       /* рад/с² ≈ 720°/с² */
            .enable_acceleration_limit = false,

            .critical_current_a     = 4.0f,           /* А — аварійне вимкнення */
            .current_timeout_ms     = 100,
            .enable_current_protection = true,

            .watchdog_timeout_ms    = 500,
            .enable_watchdog        = true,
        },

        .traj_params = {
            .type             = TRAJ_TYPE_LINEAR,
            .max_velocity     = 3.1416f,   /* рад/с ≈ 180°/с */
            .max_acceleration = 6.2832f,   /* рад/с² ≈ 360°/с² */
            .max_jerk         = 0.0f,
        },
    };
    Servo_InitFull(&servo, &srv_cfg,
                   &motor.interface,
                   &encoder.interface,
                   &brake.interface,
                   &current_driver.interface);

    HWD_UART_WriteString("-> SetPosition pi/2 (90 deg)\r\n");
//    Servo_SetPosition(&servo, 1.5708f);  /* π/2 рад = 90° */
    Servo_SetTorque(&servo, 0.3f);
    char buf[64];

    while (1) {
        Current_Sensor_Update(&current_driver.interface, 0.001f);
        Servo_Update(&servo);

        /* Вивід стану раз на 100 мс */
        static uint32_t last_print = 0;
        uint32_t now = HWD_Timer_GetMillis();
        if (now - last_print >= 100) {
            last_print = now;

            float pos = Servo_GetPosition(&servo);
            float vel = Servo_GetVelocity(&servo);
            float cur = 0.0f;
            Current_Sensor_GetCurrent(&current_driver.interface, &cur);

            int pos_i = (int)pos;
            int pos_f = (int)((pos - (float)pos_i) * 100.0f);
            int vel_i = (int)vel;
            int vel_f = (int)((vel - (float)vel_i) * 100.0f);
            int cur_i = (int)cur;
            int cur_f = (int)((cur - (float)cur_i) * 100.0f);
            if (pos_f < 0) pos_f = -pos_f;
            if (vel_f < 0) vel_f = -vel_f;
            if (cur_f < 0) cur_f = -cur_f;

            snprintf(buf, sizeof(buf), "pos:%d.%02drad vel:%d.%02drad/s cur:%d.%02dA target:%d\r\n",
                     pos_i, pos_f, vel_i, vel_f, cur_i, cur_f,
                     (int)Servo_IsAtTarget(&servo));
            HWD_UART_WriteString(buf);

            HWD_GPIO_TogglePin(&led_pin);
        }

        HWD_Timer_DelayMs(1);
    }
}
