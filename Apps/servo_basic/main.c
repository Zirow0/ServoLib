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
#include "hwd/hwd_uart.h"

#include <stdio.h>

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

    HWD_UART_WriteString("ServoLib servo_basic\r\n");

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
        /* pos-контур: положення → setpoint швидкості (рад/с) */
        .pos = {
            .kp      = 5.0f,
            .ki      = 0.0f,
            .kd      = 0.3f,
            .out_min = -10.0f,
            .out_max =  10.0f,
            .i_limit =  5.0f,
        },
        /* vel-контур: швидкість → setpoint струму (А) */
        .vel = {
            .kp      = 0.5f,
            .ki      = 5.0f,
            .kd      = 0.0f,
            .out_min = -3.0f,
            .out_max =  3.0f,
            .i_limit =  2.0f,
        },
        /* trq-контур: струм → команда двигуну (%) */
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
    Cascade_Init(&cascade, &casc_cfg, CASCADE_MODE_POS);

    /* Цільове положення: π/2 рад (90°) */
  //  cascade.target_pos = 1.5708f;
    cascade.target_current = 0.4f;

    HWD_UART_WriteString("-> target: pi/2 (90 deg)\r\n");

    char buf[64];

    while (1) {
        /* Оновлення датчиків */
        Current_Sensor_Update(&current_driver.interface);
        Brake_Update(&brake.interface);

        Position_Sensor_Update(&encoder.interface);

        float pos_rad = 0.0f;
        float vel_rad_s = 0.0f;
        float current_a = 0.0f;

        Position_Sensor_GetPosition(&encoder.interface, &pos_rad);
        Position_Sensor_GetVelocity(&encoder.interface, &vel_rad_s);
        Current_Sensor_GetCurrent(&current_driver.interface, &current_a);

        /* Каскадний PID → команда двигуну */
        uint32_t now_us = HWD_Timer_GetMicros();
        float power = Cascade_Compute(&cascade, pos_rad, vel_rad_s, current_a, now_us);
        Motor_SetPower(&motor.interface, power);

        /* Вивід стану раз на 100 мс */
        static uint32_t last_print = 0;
        uint32_t now_ms = HWD_Timer_GetMillis();
        if (now_ms - last_print >= 100) {
            last_print = now_ms;

            int pos_i = (int)pos_rad;
            int pos_f = (int)((pos_rad - (float)pos_i) * 100.0f);
            int vel_i = (int)vel_rad_s;
            int vel_f = (int)((vel_rad_s - (float)vel_i) * 100.0f);
            int cur_i = (int)current_a;
            int cur_f = (int)((current_a - (float)cur_i) * 100.0f);
            if (pos_f < 0) pos_f = -pos_f;
            if (vel_f < 0) vel_f = -vel_f;
            if (cur_f < 0) cur_f = -cur_f;

            snprintf(buf, sizeof(buf), "pos:%d.%02drad vel:%d.%02drad/s cur:%d.%02dA\r\n",
                     pos_i, pos_f, vel_i, vel_f, cur_i, cur_f);
            HWD_UART_WriteString(buf);

            HWD_GPIO_TogglePin(&led_pin);
        }

        HWD_Timer_DelayMs(1);
    }
}
