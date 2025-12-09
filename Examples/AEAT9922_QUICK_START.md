## Швидкий старт з AEAT-9922

Короткий посібник для початку роботи з магнітним енкодером AEAT-9922.

### Мінімальна конфігурація CubeMX

**SPI1:**
- Mode: Full-Duplex Master
- Prescaler: 8 (12.5 MHz)
- CPOL: Low, CPHA: 2 Edge
- Data Size: 8 Bits

**GPIO:**
- PA4: GPIO_Output (CS)
- PB0: GPIO_Output (MSEL - HIGH для SPI)

### Мінімальний код

```c
#include "drv/sensor/aeat9922.h"

extern SPI_HandleTypeDef hspi1;
AEAT9922_Driver_t encoder;

void setup() {
    AEAT9922_Config_t config = {
        .spi_config = {
            .spi_handle = &hspi1,
            .cs_port = GPIOA,
            .cs_pin = GPIO_PIN_4,
            .timeout_ms = 100
        },
        .msel_port = GPIOB,
        .msel_pin = GPIO_PIN_0,
        .abs_resolution = AEAT9922_ABS_RES_14BIT,
        .incremental_cpr = 1024,
        .interface_mode = AEAT9922_INTERFACE_SPI4_16BIT,
        .direction_ccw = false,
        .enable_incremental = false,
        .encoder_timer_handle = NULL
    };

    AEAT9922_Create(&encoder, &config);
    AEAT9922_Init(&encoder);
}

void loop() {
    float angle;
    if (AEAT9922_ReadAngle(&encoder, &angle) == SERVO_OK) {
        printf("Angle: %.2f deg\n", angle);
    }
    HAL_Delay(10);
}
```

### Детальна документація

Дивіться: `ServoLib/Doc/AEAT-9922_DRIVER_GUIDE.md`
