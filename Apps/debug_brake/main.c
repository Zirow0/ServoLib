#include "board_config.h"
#include "drv/brake/brake.h"
#include "drv/brake/gpio_brake.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_uart.h"

#include <stdio.h>

static GPIO_Brake_Driver_t brake;

static const char* brake_state_str(Brake_State_t state)
{
    switch (state) {
        case BRAKE_STATE_ENGAGED:   return "ENGAGED";
        case BRAKE_STATE_RELEASED:  return "RELEASED";
        case BRAKE_STATE_ENGAGING:  return "ENGAGING";
        case BRAKE_STATE_RELEASING: return "RELEASING";
        default:                    return "UNKNOWN";
    }
}

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

int main(void)
{
    Board_Init();

    HWD_UART_WriteString("ServoLib brake debug\r\n");

    GPIO_Brake_Config_t brk_cfg = {
        .gpio_port       = (void*)BRAKE_CTRL_GPIO_PORT,
        .gpio_pin        = BRAKE_CTRL_PIN,
        .active_high     = false,
        .engage_time_ms  = 50,
        .release_time_ms = 30,
    };
    GPIO_Brake_Create(&brake, &brk_cfg);

    char buf[48];
    uint32_t cycle = 0;

    /* Тест: відпустити → 2с → загальмувати → 2с */
    while (1) {
        HWD_UART_WriteString("-> RELEASE\r\n");
        Brake_Release(&brake.interface);
        while (!Brake_IsReleased(&brake.interface)) {
            Brake_Update(&brake.interface);
            snprintf(buf, sizeof(buf), "   state: %s\r\n",
                     brake_state_str(Brake_GetState(&brake.interface)));
            HWD_UART_WriteString(buf);
            HWD_Timer_DelayMs(10);
        }
        HWD_UART_WriteString("   state: RELEASED\r\n");
        HWD_Timer_DelayMs(2000);

        HWD_UART_WriteString("-> ENGAGE\r\n");
        Brake_Engage(&brake.interface);
        while (!Brake_IsEngaged(&brake.interface)) {
            Brake_Update(&brake.interface);
            snprintf(buf, sizeof(buf), "   state: %s\r\n",
                     brake_state_str(Brake_GetState(&brake.interface)));
            HWD_UART_WriteString(buf);
            HWD_Timer_DelayMs(10);
        }
        HWD_UART_WriteString("   state: ENGAGED\r\n");
        HWD_Timer_DelayMs(2000);

        snprintf(buf, sizeof(buf), "cycle: %lu\r\n", (unsigned long)++cycle);
        HWD_UART_WriteString(buf);

        HWD_GPIO_TogglePin(&led_pin);
    }
}
