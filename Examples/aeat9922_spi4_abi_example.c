/**
 * @file aeat9922_spi4_abi_example.c
 * @brief Приклад використання AEAT-9922 в режимі SPI4-B (24-bit CRC) + ABI
 * @author ServoCore Team
 * @date 2025
 *
 * Цей приклад демонструє одночасне використання двох режимів:
 * - SPI4-B (24-bit з CRC) для діагностики та налаштування
 * - ABI інкрементальний вихід через апаратний таймер TIM2
 */

#include "drv/position/aeat9922.h"
#include "drv/position/position.h"
#include "stm32f4xx_hal.h"

/* Глобальні змінні -----------------------------------------------------------*/

// Драйвер AEAT-9922
static AEAT9922_Driver_t encoder_driver;

// Апаратні handles
extern SPI_HandleTypeDef hspi1;   // SPI1 для AEAT-9922
extern TIM_HandleTypeDef htim2;   // TIM2 в Encoder Mode для ABI

/* Конфігурація ---------------------------------------------------------------*/

/**
 * @brief Налаштування AEAT-9922 для SPI4-B + ABI режиму
 *
 * Сценарій використання:
 * 1. SPI4-B (24-bit з CRC) - для періодичної діагностики (раз на 100ms)
 *    - Читання STATUS (RDY, MHI, MLO, MEM_Err)
 *    - Читання absolute position для верифікації
 *    - Налаштування регістрів
 *
 * 2. ABI інкрементальний вихід - для основної роботи
 *    - Апаратний підрахунок через TIM2 Encoder Mode
 *    - Zero CPU overhead
 *    - Латентність <1µs
 *    - Частота до 1 MHz
 */
void AEAT9922_Example_Init(void)
{
    Servo_Status_t status;

    // 1. Конфігурація AEAT-9922
    AEAT9922_Config_t config = {
        // Режими роботи: SPI4 + ABI одночасно
        .enabled_modes = AEAT9922_MODE_SPI4 | AEAT9922_MODE_ABI,

        // Загальні налаштування
        .general = {
            .abs_resolution = AEAT9922_ABS_RES_18BIT,  // 262144 позицій (найвища точність)
            .direction_ccw = false,                     // CW count up
            .auto_zero_on_init = true,                  // Калібрувати zero при старті
            .enable_inl_correction = true,              // Увімкнути INL корекцію для точності
            .hysteresis_deg = 0.02f                     // Гістерезис 0.02° (рекомендовано)
        },

        // SPI4-B конфігурація (24-bit з CRC для надійності)
        .spi_config = {
            .spi_config = {
                // HWD_SPI_Config_t вже налаштований через CubeMX
                .spi_handle = &hspi1,
                // SPI налаштування:
                // - Mode: Master
                // - Clock: <= 10 MHz (наприклад, 5 MHz)
                // - CPOL: 0, CPHA: 1
                // - Data Size: 8-bit
                // - NSS: Software управління
            },
            .msel_port = GPIOA,                         // MSEL на PA4
            .msel_pin = GPIO_PIN_4,
            .protocol_variant = AEAT9922_PSEL_SPI4_24BIT  // 24-bit з CRC (надійність)
        },

        // ABI інкрементальний вихід
        .abi = {
            .incremental_cpr = 4096,                    // 4096 імпульсів на оберт
            .index_width = AEAT9922_INDEX_WIDTH_90,     // Index pulse 90° width
            .index_state = AEAT9922_INDEX_STATE_360,    // Index на нульовій позиції
            .enable_incremental = true,                 // Використовувати TIM2
            .encoder_timer_handle = &htim2              // TIM2 в Encoder Mode
        },

        // UVW комутація (не використовується)
        .uvw = {
            .pole_pairs = 0
        }
    };

    // 2. Створити драйвер (прив'язує hardware callbacks)
    status = AEAT9922_Create(&encoder_driver, &config);
    if (status != SERVO_OK) {
        // Error: не вдалось створити драйвер
        Error_Handler();
    }

    // 3. Налаштування параметрів для Position_Sensor
    Position_Params_t sensor_params = {
        .type = SENSOR_TYPE_AEAT9922,
        .resolution_bits = 18,           // 18-bit абсолютна роздільність
        .min_angle = 0.0f,
        .max_angle = 360.0f,
        .update_rate = 1000              // 1 kHz update rate
    };

    // 4. Ініціалізувати інтерфейс (викликає AEAT9922_HW_Init)
    status = Position_Sensor_Init(&encoder_driver.interface, &sensor_params);
    if (status != SERVO_OK) {
        // Error: не вдалось ініціалізувати датчик
        Error_Handler();
    }

    // 5. Перевірити статус після ініціалізації
    if (!encoder_driver.status.ready) {
        // Error: датчик не готовий
        Error_Handler();
    }

    if (encoder_driver.status.magnet_high) {
        // Warning: магніт занадто близько
    }

    if (encoder_driver.status.magnet_low) {
        // Warning: магніт занадто далеко
    }
}

/**
 * @brief Основний цикл - оновлення позиції
 *
 * Викликати на частоті 1 kHz (кожну 1 ms)
 */
void AEAT9922_Example_Update(void)
{
    // Оновити лічильник з TIM2 (якщо використовується ABI з таймером)
    AEAT9922_UpdateIncrementalCount(&encoder_driver);

    // Оновити Position Sensor (читає SPI4-B, розраховує velocity, prediction)
    Position_Sensor_Update(&encoder_driver.interface);

    // Отримати поточну позицію
    float position_deg;
    Position_Sensor_GetPosition(&encoder_driver.interface, &position_deg);

    // Отримати швидкість
    float velocity_dps;
    Position_Sensor_GetVelocity(&encoder_driver.interface, &velocity_dps);

    // Отримати абсолютну позицію (з multi-turn)
    float absolute_position;
    Position_Sensor_GetAbsolutePosition(&encoder_driver.interface, &absolute_position);

    // Отримати передбачену позицію (екстраполяція між зчитуваннями)
    float predicted_position;
    Position_Sensor_GetPredictedPosition(&encoder_driver.interface, &predicted_position);

    // Використовувати дані...
    // (наприклад, передати в PID controller)
}

/**
 * @brief Періодична діагностика через SPI4-B
 *
 * Викликати раз на 100 ms для моніторингу статусу
 */
void AEAT9922_Example_Diagnostics(void)
{
    Servo_Status_t status;

    // Прочитати статус енкодера
    status = AEAT9922_ReadStatus(&encoder_driver);
    if (status != SERVO_OK) {
        // Error: не вдалось прочитати статус
        return;
    }

    // Перевірити прапорці
    if (encoder_driver.status.magnet_high) {
        // Warning: магніт занадто близько (>100 mT для On-Axis)
        // Дія: збільшити відстань між магнітом та датчиком
    }

    if (encoder_driver.status.magnet_low) {
        // Warning: магніт занадто далеко (<45 mT для On-Axis)
        // Дія: зменшити відстань або використати сильніший магніт
    }

    if (encoder_driver.status.memory_error) {
        // Critical Error: помилка пам'яті EEPROM (CRC check failed)
        // Дія: спробувати перепрограмувати EEPROM або замінити датчик
        Error_Handler();
    }

    if (!encoder_driver.status.ready) {
        // Critical Error: датчик не готовий
        Error_Handler();
    }

    // Вивести лічильник помилок SPI
    if (encoder_driver.error_count > 0) {
        // Warning: виявлені помилки SPI
        // Може бути проблема з підключенням або перешкодами
    }
}

/**
 * @brief Калібрування нульової позиції
 *
 * Викликати коли вал знаходиться в бажаній нульовій позиції
 */
void AEAT9922_Example_CalibrateZero(void)
{
    Servo_Status_t status;

    // Виконати zero reset калібрування
    status = AEAT9922_CalibrateZero(&encoder_driver);
    if (status != SERVO_OK) {
        // Error: калібрування провалено
        Error_Handler();
    }

    // Опціонально: зберегти в EEPROM для збереження після вимкнення
    status = AEAT9922_ProgramEEPROM(&encoder_driver);
    if (status != SERVO_OK) {
        // Error: не вдалось зберегти в EEPROM
        Error_Handler();
    }
}

/**
 * @brief Callback для Index pulse (опціонально)
 *
 * Налаштувати GPIO EXTI interrupt на пін I (Index)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Якщо це Index pulse від AEAT-9922
    if (GPIO_Pin == GPIO_PIN_XX) {  // Замінити на фактичний пін I
        // Викликати callback для підрахунку обертів
        AEAT9922_IndexPulseCallback(&encoder_driver);
    }
}

/**
 * @brief Приклад використання в control loop
 */
void Control_Loop_Example(void)
{
    // Цикл управління на частоті 1 kHz
    while (1) {
        // 1. Оновити позицію датчика
        AEAT9922_Example_Update();

        // 2. Отримати поточну позицію
        float current_position;
        Position_Sensor_GetPosition(&encoder_driver.interface, &current_position);

        // 3. PID регулювання
        // float error = target_position - current_position;
        // float control_output = PID_Calculate(error);

        // 4. Застосувати управління до мотора
        // Motor_SetPower_DC(&motor, control_output);

        // Затримка 1 ms (1 kHz)
        HAL_Delay(1);
    }
}

/* Налаштування TIM2 для Encoder Mode (через CubeMX) -------------------------*/
/*
 * TIM2 Configuration:
 * - Mode: Encoder Mode TI1 and TI2
 * - Encoder Mode: Encoder Mode 3 (Count on both TI1 and TI2 edges)
 * - Counter Settings:
 *   - Prescaler: 0
 *   - Counter Mode: Up
 *   - Counter Period: 65535 (16-bit) або 4294967295 (32-bit якщо TIM2/TIM5)
 *   - auto-reload preload: Enable
 *
 * Pins:
 * - TIM2_CH1 (PA15): підключити до A (AEAT-9922 pin 24)
 * - TIM2_CH2 (PB3):  підключити до B (AEAT-9922 pin 23)
 *
 * Opціонально - Index pulse:
 * - GPIO Input (наприклад PA10): підключити до I (AEAT-9922 pin 20)
 * - Налаштувати EXTI interrupt на rising edge
 */

/* Налаштування SPI1 (через CubeMX) -------------------------------------------*/
/*
 * SPI1 Configuration:
 * - Mode: Full-Duplex Master
 * - Hardware NSS Signal: Disable (Software управління через GPIO)
 * - Frame Format: Motorola
 * - Data Size: 8 Bits
 * - First Bit: MSB First
 * - Prescaler: 16 або більше (щоб частота <= 10 MHz)
 *   - Якщо APB2 = 84 MHz, то prescaler = 16 -> 5.25 MHz
 * - Clock Polarity (CPOL): Low (0)
 * - Clock Phase (CPHA): 2 Edge (1)
 * - CRC Calculation: Disabled
 * - NSS Signal Type: Software
 *
 * Pins:
 * - SPI1_SCK (PA5):  підключити до M2 (AEAT-9922 pin 11, SCK)
 * - SPI1_MISO (PA6): підключити до M3 (AEAT-9922 pin 12, MISO)
 * - SPI1_MOSI (PA7): підключити до M1 (AEAT-9922 pin 10, MOSI)
 * - CS (PA4):        підключити до M0 (AEAT-9922 pin 9, NCS) - Software управління
 *
 * MSEL Pin:
 * - GPIO Output (PA4): підключити до MSEL (AEAT-9922 pin 19)
 */
