#include "board_config.h"
#include "drv/position/position.h"
#include "drv/position/aeat9922.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>  /* snprintf */

static AEAT9922_Driver_t encoder;

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

    AEAT9922_Config_t enc_cfg = {
        .enabled_modes = AEAT9922_MODE_SPI4,
        .general = {
            .abs_resolution     = AEAT9922_ABS_RES_18BIT,
            .direction_ccw      = false,
            .auto_zero_on_init  = false,
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

    Position_Params_t params = {
        .type        = SENSOR_TYPE_ENCODER_MAG,
        .min_angle   = 0.0f,
        .max_angle   = 360.0f,
        .update_rate = 1000,
    };
    Position_Sensor_Init(&encoder.interface, &params);

    char buf[48];

    while (1) {
        Position_Sensor_Update(&encoder.interface);

        float pos = 0.0f, vel = 0.0f;
        Position_Sensor_GetPosition(&encoder.interface, &pos);
        Position_Sensor_GetVelocity(&encoder.interface, &vel);

        /* Вивід: "pos:123.45 vel:-67.89\r\n" */
        int pos_int = (int)pos;
        int pos_frac = (int)((pos - (float)pos_int) * 100.0f);
        int vel_int = (int)vel;
        int vel_frac = (int)((vel - (float)vel_int) * 100.0f);
        if (vel_frac < 0) { vel_frac = -vel_frac; }
        if (pos_frac < 0) { pos_frac = -pos_frac; }

        snprintf(buf, sizeof(buf), "pos:%d.%02d vel:%d.%02d\r\n",
                 pos_int, pos_frac, vel_int, vel_frac);
        HWD_UART_WriteString(buf);

        HWD_GPIO_TogglePin(&led_pin);
        HWD_Timer_DelayMs(100);
    }
}
