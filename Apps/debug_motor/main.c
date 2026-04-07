#include "board_config.h"
#include "drv/motor/motor.h"
#include "drv/motor/pwm.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>

static PWM_Motor_Driver_t motor;

static const char* motor_state_str(Motor_State_t state)
{
    switch (state) {
        case MOTOR_STATE_IDLE:    return "IDLE";
        case MOTOR_STATE_RUNNING: return "RUNNING";
        case MOTOR_STATE_BRAKING: return "BRAKING";
        case MOTOR_STATE_ERROR:   return "ERROR";
        default:                  return "UNKNOWN";
    }
}

static const char* motor_dir_str(Motor_Direction_t dir)
{
    switch (dir) {
        case MOTOR_DIR_FORWARD:  return "FWD";
        case MOTOR_DIR_BACKWARD: return "BWD";
        case MOTOR_DIR_BRAKE:    return "BRK";
        default:                 return "???";
    }
}

static HWD_PWM_Handle_t   pwm_fwd;

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

    /* ── Двигун ─────────────────────────────────────────────────────────── */
    PWM_Motor_Config_t mot_cfg = {
        .type     = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,
        .pwm_fwd  = &pwm_fwd,
        .pwm_bwd  = NULL,
        .gpio_dir = (void*)MOTOR_DIR_GPIO_PORT,
        .gpio_pin = MOTOR_DIR_PIN,
    };
    PWM_Motor_Create(&motor, &mot_cfg);

    Motor_Params_t params = {
        .type             = MOTOR_TYPE_DC_PWM,
        .max_power        = 100.0f,
        .min_power        = 5.0f,
        .invert_direction = false,
    };
    Motor_Init(&motor.interface, &params);

    HWD_UART_WriteString("ServoLib motor debug\r\n");

    char buf[64];
    uint32_t cycle = 0;
    Motor_Stats_t stats;

    /* ── Тест: вперед → стоп → назад → стоп ────────────────────────────── */
    while (1) {
        HWD_UART_WriteString("-> FWD 50%\r\n");
        Motor_SetPower_DC(&motor.interface, 50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_GetStats(&motor.interface, &stats);
        snprintf(buf, sizeof(buf), "   state:%s dir:%s pwr:%d%%\r\n",
                 motor_state_str(stats.state),
                 motor_dir_str(stats.direction),
                 (int)stats.current_power);
        HWD_UART_WriteString(buf);

        HWD_UART_WriteString("-> STOP\r\n");
        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        HWD_UART_WriteString("-> BWD 50%\r\n");
        Motor_SetPower_DC(&motor.interface, -50.0f);
        HWD_Timer_DelayMs(2000);

        Motor_GetStats(&motor.interface, &stats);
        snprintf(buf, sizeof(buf), "   state:%s dir:%s pwr:%d%%\r\n",
                 motor_state_str(stats.state),
                 motor_dir_str(stats.direction),
                 (int)stats.current_power);
        HWD_UART_WriteString(buf);

        HWD_UART_WriteString("-> STOP\r\n");
        Motor_Stop(&motor.interface);
        HWD_Timer_DelayMs(1000);

        Motor_GetStats(&motor.interface, &stats);
        snprintf(buf, sizeof(buf), "cycle:%lu starts:%lu run:%lums err:%lu\r\n",
                 (unsigned long)++cycle,
                 (unsigned long)stats.total_starts,
                 (unsigned long)stats.run_time_ms,
                 (unsigned long)stats.error_count);
        HWD_UART_WriteString(buf);

        HWD_GPIO_TogglePin(&led_pin);
    }
}
