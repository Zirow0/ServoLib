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
    Position_Sensor_Init(&encoder.interface);

    char buf[64];

    while (1) {
        Position_Sensor_Update(&encoder.interface);

        float pos_rad = 0.0f, vel_rad_s = 0.0f, abs_pos_rad = 0.0f;
        Position_Sensor_GetPosition(&encoder.interface, &pos_rad);
        Position_Sensor_GetVelocity(&encoder.interface, &vel_rad_s);
        Position_Sensor_GetAbsolutePosition(&encoder.interface, &abs_pos_rad);

        /* Конвертація для зручного виводу: рад → градуси */
        int pos_i  = (int)(pos_rad * 57.2958f);
        int pos_f  = (int)((pos_rad * 57.2958f - (float)pos_i) * 100.0f);
        int vel_i  = (int)(vel_rad_s * 57.2958f);
        int vel_f  = (int)((vel_rad_s * 57.2958f - (float)vel_i) * 100.0f);
        int abs_i  = (int)(abs_pos_rad * 57.2958f);
        if (pos_f < 0) { pos_f = -pos_f; }
        if (vel_f < 0) { vel_f = -vel_f; }

        /* Вивід: "pos:123.45deg vel:-67.89deg/s a_pos:720deg\r\n" */
        snprintf(buf, sizeof(buf), "pos:%d.%02ddeg vel:%d.%02ddeg/s a_pos:%ddeg\r\n",
                 pos_i, pos_f, vel_i, vel_f, abs_i);
        HWD_UART_WriteString(buf);

        HWD_GPIO_TogglePin(&led_pin);
        HWD_Timer_DelayMs(100);
    }
}
