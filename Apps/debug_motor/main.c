#include "board_config.h"
#include "drv/motor/motor.h"
#include "drv/motor/pwm.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"

static PWM_Motor_Driver_t motor;
static HWD_PWM_Handle_t   pwm_fwd;
static HWD_PWM_Handle_t   pwm_bwd;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

int main(void)
{
    Board_Init();

    /* ── Ініціалізація PWM каналів ──────────────────────────────────────── */
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

    /* ── Двигун ─────────────────────────────────────────────────────────── */
    PWM_Motor_Config_t mot_cfg = {
        .type    = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd = &pwm_fwd,
        .pwm_bwd = &pwm_bwd,
        .gpio_dir = NULL,
    };
    PWM_Motor_Create(&motor, &mot_cfg);

    Motor_Params_t params = {
        .type             = MOTOR_TYPE_DC_PWM,
        .max_power        = 100.0f,
        .min_power        = 5.0f,
        .invert_direction = false,
    };
    Motor_Init(&motor.interface, &params);

    /* ── Тест: вперед → стоп → назад → стоп ────────────────────────────── */
    while (1) {
        Motor_SetPower_DC(&motor.interface, 50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        Motor_SetPower_DC(&motor.interface, -50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        HWD_GPIO_TogglePin(&led_pin);
    }
}
