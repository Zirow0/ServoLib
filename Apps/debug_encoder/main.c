#include "board_config.h"
#include "drv/position/position.h"
#include "drv/position/aeat9922.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"

static AEAT9922_Driver_t encoder;

int main(void)
{
    Board_Init();

    AEAT9922_Config_t enc_cfg = {
        .spi_config = BOARD_SPI_AEAT9922_CONFIG,
        .msel_port  = BOARD_AEAT9922_MSEL_PORT,
        .msel_pin   = BOARD_AEAT9922_MSEL_PIN,
        .abs_resolution   = AEAT9922_ABS_RES_18BIT,
        .interface_mode   = AEAT9922_INTERFACE_SPI4_24BIT,
        .direction_ccw    = false,
        .enable_incremental = false,
    };
    AEAT9922_Create(&encoder, &enc_cfg);

    Position_Params_t params = {
        .type            = SENSOR_TYPE_AEAT9922,
        .resolution_bits = 18,
        .min_angle       = 0.0f,
        .max_angle       = 360.0f,
        .update_rate     = 1000,
    };
    Position_Sensor_Init(&encoder.interface, &params);

    while (1) {
        Position_Sensor_Update(&encoder.interface);

        float pos = 0.0f, vel = 0.0f;
        Position_Sensor_GetPosition(&encoder.interface, &pos);
        Position_Sensor_GetVelocity(&encoder.interface, &vel);

        /* TODO: вивести pos/vel через UART або зберегти у кільцевий буфер */
        HWD_GPIO_TogglePin(BOARD_LED_PORT, BOARD_LED_PIN);
        HWD_Timer_DelayMs(100);
    }
}
