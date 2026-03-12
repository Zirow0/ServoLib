#include "board_config.h"
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/position/aeat9922.h"
#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_timer.h"

static PWM_Motor_Driver_t  motor;
static AEAT9922_Driver_t   encoder;
static GPIO_Brake_Driver_t brake;
static Servo_Controller_t  servo;

int main(void)
{
    Board_Init();

    /* ── Двигун ─────────────────────────────────────────────────────────── */
    PWM_Motor_Config_t mot_cfg = {
        .type    = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd = &BOARD_PWM_FWD_HANDLE,
        .pwm_bwd = &BOARD_PWM_BWD_HANDLE,
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
        .spi_config       = BOARD_SPI_AEAT9922_CONFIG,
        .msel_port        = BOARD_AEAT9922_MSEL_PORT,
        .msel_pin         = BOARD_AEAT9922_MSEL_PIN,
        .abs_resolution   = AEAT9922_ABS_RES_18BIT,
        .interface_mode   = AEAT9922_INTERFACE_SPI4_24BIT,
    };
    AEAT9922_Create(&encoder, &enc_cfg);
    Position_Params_t enc_params = {
        .type            = SENSOR_TYPE_AEAT9922,
        .resolution_bits = 18,
        .min_angle       = 0.0f,
        .max_angle       = 360.0f,
        .update_rate     = 1000,
    };
    Position_Sensor_Init(&encoder.interface, &enc_params);

    /* ── Гальмо ──────────────────────────────────────────────────────────── */
    GPIO_Brake_Config_t brk_cfg = {
        .gpio_port       = BOARD_BRAKE_PORT,
        .gpio_pin        = BOARD_BRAKE_PIN,
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
        Servo_Update(&servo);   /* виклик 1 раз на 1 мс */
        HWD_Timer_DelayMs(1);
    }
}
