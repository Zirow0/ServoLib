#include "board_config.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>

/* Часові інтервали --------------------------------------------------------*/
#define SENSOR_PERIOD_US    100U    /* 10 кГц */
#define CONTROL_PERIOD_US   1000U   /* 1 кГц  */
#define TELEM_PERIOD_US     200000U /* 5 Гц   */

#define SENSOR_DT_S         (SENSOR_PERIOD_US * 1e-6f)

static ACS712_Driver_t  current_driver;
static HWD_ADC_Handle_t current_adc;

int main(void)
{
    Board_Init();

    HWD_UART_WriteString("ServoLib current debug\r\n");

    static const HWD_ADC_Config_t adc_cfg = {
        .adc_base  = CURRENT_ADC_PERIPH,
        .rcc_adc   = CURRENT_ADC_RCC,
        .rcc_gpio  = CURRENT0_ADC_GPIO_RCC,
        .gpio_port = CURRENT0_ADC_GPIO_PORT,
        .gpio_pin  = CURRENT0_ADC_GPIO_PIN,
        .channel   = CURRENT0_ADC_CHANNEL,
        .vref_v    = CURRENT_ADC_VREF_V,
    };
    HWD_ADC_Init(&current_adc, &adc_cfg);
    HWD_ADC_StartScan();

    static const ACS712_Config_t acs_cfg = {
        .variant       = ACS712_30A,
        .adc           = &current_adc,
        .divider_ratio = 0.65f,
    };
    static const Current_Params_t current_params = {
        .overcurrent_threshold_a = 4.0f,
        .process_noise_q         = 0.001f,
        .measurement_noise_r     = 0.02f,
    };
    ACS712_Create(&current_driver, &acs_cfg, &current_params);

    HWD_UART_WriteString("Calibrating...\r\n");
    Current_Sensor_Calibrate(&current_driver.interface);
    HWD_UART_WriteString("Ready\r\n");

    uint32_t last_sensor  = HWD_Timer_GetMicros();
    uint32_t last_control = last_sensor;
    uint32_t last_telem   = last_sensor;

    char buf[64];

    while (1) {
        uint32_t now = HWD_Timer_GetMicros();

        /* ── 10 кГц: зчитування датчика ─────────────────────────────────── */
        if (now - last_sensor >= SENSOR_PERIOD_US) {
            last_sensor += SENSOR_PERIOD_US;
            Current_Sensor_Update(&current_driver.interface, SENSOR_DT_S);
        }

        /* ── 1 кГц: обробка та команди ──────────────────────────────────── */
        if (now - last_control >= CONTROL_PERIOD_US) {
            last_control += CONTROL_PERIOD_US;
            /* placeholder: тут буде каскад PID, команди мотору тощо */
        }

        /* ── 5 Гц: телеметрія ────────────────────────────────────────────── */
        if (now - last_telem >= TELEM_PERIOD_US) {
            last_telem += TELEM_PERIOD_US;

            float current_a = 0.0f;
            float peak_a    = 0.0f;
            Current_Sensor_GetCurrent(&current_driver.interface, &current_a);
            Current_Sensor_GetPeakCurrent(&current_driver.interface, &peak_a);
            bool  overcurrent = Current_Sensor_IsOvercurrent(&current_driver.interface);

            int cur_int  = (int)current_a;
            int cur_frac = (int)((current_a - (float)cur_int) * 100.0f);
            int pk_int   = (int)peak_a;
            int pk_frac  = (int)((peak_a - (float)pk_int) * 100.0f);
            if (cur_frac < 0) { cur_frac = -cur_frac; }

            snprintf(buf, sizeof(buf), "cur:%d.%02dA peak:%d.%02dA oc:%d\r\n",
                     cur_int, cur_frac, pk_int, pk_frac, (int)overcurrent);
            HWD_UART_WriteString(buf);
        }
    }
}
