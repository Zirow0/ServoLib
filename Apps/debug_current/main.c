#include "board_config.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>  /* snprintf */

static ACS712_Driver_t  current_driver;
static HWD_ADC_Handle_t current_adc;

int main(void)
{
    Board_Init();

    HWD_UART_WriteString("ServoLib current debug\r\n");

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
        .divider_ratio           = 0.694f,
        .overcurrent_threshold_a = 4.0f,
        .ema_alpha               = 0.1f,
    };
    ACS712_Create(&current_driver, &acs_cfg);

    /* Калібрування при нульовому струмі */
    HWD_UART_WriteString("Calibrating...\r\n");
    Current_Sensor_Calibrate(&current_driver.interface);
    HWD_UART_WriteString("Ready\r\n");

    char buf[64];

    while (1) {
        Current_Sensor_Update(&current_driver.interface);

        float current_a = 0.0f;
        float peak_a    = 0.0f;
        Current_Sensor_GetCurrent(&current_driver.interface, &current_a);
        Current_Sensor_GetPeakCurrent(&current_driver.interface, &peak_a);
        bool  overcurrent = Current_Sensor_IsOvercurrent(&current_driver.interface);

        /* Вивід: "cur:1.23A peak:2.34A oc:0\r\n" */
        int cur_int  = (int)current_a;
        int cur_frac = (int)((current_a - (float)cur_int) * 100.0f);
        int pk_int   = (int)peak_a;
        int pk_frac  = (int)((peak_a - (float)pk_int) * 100.0f);
        if (cur_frac < 0) { cur_frac = -cur_frac; }

        snprintf(buf, sizeof(buf), "cur:%d.%02dA peak:%d.%02dA oc:%d\r\n",
                 cur_int, cur_frac, pk_int, pk_frac, (int)overcurrent);
        HWD_UART_WriteString(buf);

        HWD_Timer_DelayMs(200U);  /* 5 Hz */
    }
}
