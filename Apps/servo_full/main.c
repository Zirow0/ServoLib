#include "board_config.h"
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/position/aeat9922.h"
#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_timer.h"

static PWM_Motor_Driver_t  motor;
static HWD_PWM_Handle_t    pwm_fwd;
static HWD_PWM_Handle_t    pwm_bwd;
static AEAT9922_Driver_t   encoder;
static GPIO_Brake_Driver_t brake;
static Servo_Controller_t  servo;

int main(void)
{
    Board_Init();

    /* ── PWM канали ──────────────────────────────────────────────────────── */
    HWD_PWM_Config_t fwd_cfg = {
        .frequency  = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel    = HWD_PWM_CHANNEL_1,
        .hw_handle  = (void*)MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_OC_FWD,
    };
    HWD_PWM_Init(&pwm_fwd, &fwd_cfg);

    HWD_PWM_Config_t bwd_cfg = {
        .frequency  = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel    = HWD_PWM_CHANNEL_2,
        .hw_handle  = (void*)MOTOR_PWM_TIMER,
        .hw_channel = MOTOR_PWM_OC_BWD,
    };
    HWD_PWM_Init(&pwm_bwd, &bwd_cfg);

    /* ── Двигун ──────────────────────────────────────────────────────────── */
    PWM_Motor_Config_t mot_cfg = {
        .type    = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd = &pwm_fwd,
        .pwm_bwd = &pwm_bwd,
    };
    PWM_Motor_Create(&motor, &mot_cfg);

    Motor_Params_t mot_params = {
        .type      = MOTOR_TYPE_DC_PWM,
        .max_power = 100.0f,
        .min_power = 5.0f,
    };
    Motor_Init(&motor.interface, &mot_params);

    /* ── Датчик ──────────────────────────────────────────────────────────── */
    AEAT9922_Config_t enc_cfg = {
        .enabled_modes = AEAT9922_MODE_SPI4,
        .general = {
            .abs_resolution    = AEAT9922_ABS_RES_18BIT,
            .direction_ccw     = false,
            .auto_zero_on_init = false,
        },
        .spi_config = {
            .spi_config = {
                .spi_handle = (void*)ENCODER_SPI,
                .cs_port    = (void*)ENCODER_CS_GPIO_PORT,
                .cs_pin     = ENCODER_CS_PIN,
                .timeout_ms = 10,
            },
            .msel_port        = (void*)ENCODER_MSEL_GPIO_PORT,
            .msel_pin         = ENCODER_MSEL_PIN,
            .protocol_variant = AEAT9922_PSEL_SPI4_24BIT,
        },
    };
    AEAT9922_Create(&encoder, &enc_cfg);

    Position_Params_t enc_params = {
        .type        = SENSOR_TYPE_ENCODER_MAG,
        .min_angle   = 0.0f,
        .max_angle   = 360.0f,
        .update_rate = 1000,
    };
    Position_Sensor_Init(&encoder.interface, &enc_params);

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
        /* TODO: заповнити PID, safety, trajectory параметри */
    };
    Servo_InitFull(&servo, &srv_cfg,
                   &motor.interface,
                   &encoder.interface,
                   &brake.interface);

    Servo_SetPosition(&servo, 90.0f);

    while (1) {
        Servo_Update(&servo);
        HWD_Timer_DelayMs(1);
    }
}
