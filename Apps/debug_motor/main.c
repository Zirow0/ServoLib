#include "board_config.h"
#include "drv/motor/motor.h"
#include "drv/motor/pwm.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"

static PWM_Motor_Driver_t motor;

int main(void)
{
    Board_Init();

    PWM_Motor_Config_t mot_cfg = {
        .type    = PWM_MOTOR_TYPE_DUAL_PWM,
        .pwm_fwd = &BOARD_PWM_FWD_HANDLE,
        .pwm_bwd = &BOARD_PWM_BWD_HANDLE,
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

    /* Простий тест: вперед-зупинка-назад */
    while (1) {
        Motor_SetPower_DC(&motor.interface, 50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        Motor_SetPower_DC(&motor.interface, -50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        HWD_GPIO_TogglePin(BOARD_LED_PORT, BOARD_LED_PIN);
    }
}
