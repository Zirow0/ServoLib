#include "board_config.h"
#include "ctrl/servo.h"
#include "drv/motor/pwm.h"
#include "drv/position/incremental_encoder.h"
#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>

static PWM_Motor_Driver_t          motor;
static HWD_PWM_Handle_t            pwm_fwd;
static Incremental_Encoder_Driver_t encoder;
static GPIO_Brake_Driver_t         brake;
static Servo_Controller_t          servo;

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

    /* ── PWM канал ───────────────────────────────────────────────────────── */
    HWD_PWM_Config_t fwd_cfg = {
        .frequency  = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .channel    = HWD_PWM_CHANNEL_1,
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
        .type             = MOTOR_TYPE_DC_PWM,
        .max_power        = 100.0f,
        .min_power        = 5.0f,
        .invert_direction = false,
    };
    Motor_Init(&motor.interface, &mot_params);

    /* ── Інкрементальний енкодер ─────────────────────────────────────────── */
    Incremental_Encoder_Config_t enc_cfg = {
        .cpr = ENCODER_CPR,
        .hw  = {
            .timer_base  = ENCODER_TIMER_BASE,
            .rcc_timer   = ENCODER_TIMER_RCC,
            .gpio_port_a = ENCODER_GPIO_PORT_A,
            .gpio_pin_a  = ENCODER_GPIO_PIN_A,
            .rcc_gpio_a  = ENCODER_GPIO_RCC_A,
            .gpio_port_b = ENCODER_GPIO_PORT_B,
            .gpio_pin_b  = ENCODER_GPIO_PIN_B,
            .rcc_gpio_b  = ENCODER_GPIO_RCC_B,
            .gpio_af     = ENCODER_GPIO_AF,
            .invert_a    = false,
            .invert_b    = false,
        },
    };
    Incremental_Encoder_Create(&encoder, &enc_cfg);

    Position_Params_t enc_params = {
        .type        = SENSOR_TYPE_ENCODER_OPT,
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

    HWD_UART_WriteString("-> SetPosition 90.0\r\n");
    Servo_SetPosition(&servo, 90.0f);

    char buf[64];

    while (1) {
        Servo_Update(&servo);

        /* Вивід стану раз на 100 мс */
        static uint32_t last_print = 0;
        uint32_t now = HWD_Timer_GetMillis();
        if (now - last_print >= 100) {
            last_print = now;

            float pos = Servo_GetPosition(&servo);
            float vel = Servo_GetVelocity(&servo);

            int pos_i = (int)pos;
            int pos_f = (int)((pos - (float)pos_i) * 100.0f);
            int vel_i = (int)vel;
            int vel_f = (int)((vel - (float)vel_i) * 100.0f);
            if (pos_f < 0) pos_f = -pos_f;
            if (vel_f < 0) vel_f = -vel_f;

            snprintf(buf, sizeof(buf), "pos:%d.%02d vel:%d.%02d target:%d\r\n",
                     pos_i, pos_f, vel_i, vel_f,
                     (int)Servo_IsAtTarget(&servo));
            HWD_UART_WriteString(buf);

            HWD_GPIO_TogglePin(&led_pin);
        }

        HWD_Timer_DelayMs(1);
    }
}
