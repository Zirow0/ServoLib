#include "board_config.h"
#include "drv/position/position.h"
#include "drv/position/incremental_encoder.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>  /* snprintf */

static Incremental_Encoder_Driver_t encoder;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

int main(void)
{
    Board_Init();

    HWD_UART_WriteString("ServoLib encoder debug\r\n");

    static const Incremental_Encoder_HW_t enc_hw = {
        .gpio_port_a = ENCODER_GPIO_PORT_A,
        .gpio_pin_a  = ENCODER_GPIO_PIN_A,
        .gpio_af_a   = ENCODER_GPIO_AF,
        .gpio_port_b = ENCODER_GPIO_PORT_B,
        .gpio_pin_b  = ENCODER_GPIO_PIN_B,
        .timer_base  = ENCODER_TIMER_BASE,
        .timer_rcc   = ENCODER_TIMER_RCC,
        .ic_channel  = 0U,  /* TIM_IC1 = CH1 (0-based, відповідає enum tim_ic_id) */
    };
    Incremental_Encoder_Create(&encoder, ENCODER_CPR, &enc_hw);
    Position_Sensor_Init(&encoder.interface, true);

    char buf[48];

    while (1) {
        Position_Sensor_Update(&encoder.interface);

        float pos = 0.0f, vel = 0.0f, pos_absolut = 0.0f;
        Position_Sensor_GetPosition(&encoder.interface, &pos);
        Position_Sensor_GetVelocity(&encoder.interface, &vel);
        Position_Sensor_GetAbsolutePosition(&encoder.interface, &pos_absolut);
        /* Вивід: "pos:123.45 vel:-67.89\r\n" */
        int pos_int = (int)pos;
        int pos_frac = (int)((pos - (float)pos_int) * 100.0f);
        int vel_int = (int)vel;
        int vel_frac = (int)((vel - (float)vel_int) * 100.0f);
        if (vel_frac < 0) { vel_frac = -vel_frac; }
        if (pos_frac < 0) { pos_frac = -pos_frac; }

        snprintf(buf, sizeof(buf), "pos:%d.%02d vel:%d.%02d a_pos:%d\r\n",
                 pos_int, pos_frac, vel_int, vel_frac, (int)pos_absolut);
        HWD_UART_WriteString(buf);

        HWD_GPIO_TogglePin(&led_pin);
        HWD_Timer_DelayMs(100);
    }
}
