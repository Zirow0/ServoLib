#include "board_config.h"
#include "drv/brake/brake.h"
#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"

static GPIO_Brake_Driver_t brake;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

int main(void)
{
    Board_Init();

    GPIO_Brake_Config_t brk_cfg = {
        .gpio_port       = (void*)BRAKE_CTRL_GPIO_PORT,
        .gpio_pin        = BRAKE_CTRL_PIN,
        .active_high     = false,
        .engage_time_ms  = 50,
        .release_time_ms = 30,
    };
    GPIO_Brake_Create(&brake, &brk_cfg);

    /* Тест: відпустити → 2с → загальмувати → 2с */
    while (1) {
        Brake_Release(&brake.interface);
        while (!Brake_IsReleased(&brake.interface)) {
            Brake_Update(&brake.interface);
        }
        HWD_Timer_DelayMs(2000);

        Brake_Engage(&brake.interface);
        while (!Brake_IsEngaged(&brake.interface)) {
            Brake_Update(&brake.interface);
        }
        HWD_Timer_DelayMs(2000);

        HWD_GPIO_TogglePin(&led_pin);
    }
}
