/**
 * @file aeat9922_quick_start.c
 * @brief Швидкий старт з AEAT-9922 (мінімальний приклад)
 * @author ServoCore Team
 * @date 2025
 */

#include "drv/position/aeat9922.h"
#include "drv/position/position.h"

/* Глобальні змінні */
AEAT9922_Driver_t encoder;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;

/**
 * @brief Мінімальна ініціалізація AEAT-9922
 */
void QuickStart_Init(void)
{
    // 1. Конфігурація
    AEAT9922_Config_t config = {
        .enabled_modes = AEAT9922_MODE_SPI4 | AEAT9922_MODE_ABI,

        .general = {
            .abs_resolution = AEAT9922_ABS_RES_18BIT,
            .direction_ccw = false,
            .auto_zero_on_init = false,
            .enable_inl_correction = true,
            .hysteresis_deg = 0.02f
        },

        .spi_config = {
            .spi_config.spi_handle = &hspi1,
            .msel_port = GPIOA,
            .msel_pin = GPIO_PIN_4,
            .protocol_variant = AEAT9922_PSEL_SPI4_24BIT
        },

        .abi = {
            .incremental_cpr = 4096,
            .index_width = AEAT9922_INDEX_WIDTH_90,
            .index_state = AEAT9922_INDEX_STATE_360,
            .enable_incremental = true,
            .encoder_timer_handle = &htim2
        }
    };

    // 2. Створити та ініціалізувати
    AEAT9922_Create(&encoder, &config);

    Position_Params_t params = {
        .type = SENSOR_TYPE_AEAT9922,
        .min_angle = 0.0f,
        .max_angle = 360.0f,
        .update_rate = 1000
    };

    Position_Sensor_Init(&encoder.interface, &params);
}

/**
 * @brief Оновлення в control loop (1 kHz)
 */
void QuickStart_Update(void)
{
    float position, velocity;

    // Оновити датчик
    AEAT9922_UpdateIncrementalCount(&encoder);
    Position_Sensor_Update(&encoder.interface);

    // Отримати дані
    Position_Sensor_GetPosition(&encoder.interface, &position);
    Position_Sensor_GetVelocity(&encoder.interface, &velocity);

    // Використати position та velocity...
}

/**
 * @brief Діагностика (100 ms)
 */
void QuickStart_Diagnostics(void)
{
    AEAT9922_ReadStatus(&encoder);

    if (encoder.status.magnet_high || encoder.status.magnet_low) {
        // Магніт не в оптимальній позиції
    }
}
