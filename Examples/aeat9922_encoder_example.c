/**
 * @file aeat9922_encoder_example.c
 * @brief Приклад використання драйвера AEAT-9922 магнітного енкодера
 * @author ServoCore Team
 * @date 2025
 *
 * Цей приклад показує як використовувати високоточний магнітний енкодер
 * AEAT-9922 для зчитування абсолютної позиції через SPI інтерфейс.
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv/sensor/aeat9922.h"
#include "Board/STM32F411/board_config.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define ENCODER_SAMPLE_TIME_MS    10    // Період оновлення (100 Hz)

/* Private variables ---------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;
AEAT9922_Driver_t encoder;

/* Private function prototypes -----------------------------------------------*/
void Encoder_AEAT9922_Init(void);
void Encoder_Simple_Test(void);
void Encoder_Continuous_Read(void);
void Encoder_Diagnostics(void);
void Encoder_Calibration_Example(void);

/* Main Example Functions ----------------------------------------------------*/

/**
 * @brief Головна функція прикладу
 *
 * Додайте цей код до вашого main.c:
 *
 * int main(void)
 * {
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *     MX_SPI1_Init();
 *
 *     Encoder_AEAT9922_Init();
 *
 *     while (1)
 *     {
 *         Encoder_Example_Loop();
 *         HAL_Delay(ENCODER_SAMPLE_TIME_MS);
 *     }
 * }
 */
void Encoder_Example_Loop(void)
{
    // Варіант 1: Просте зчитування кута
    Encoder_Simple_Test();

    // Варіант 2: Безперервне зчитування з виведенням
    // Encoder_Continuous_Read();

    // Варіант 3: Діагностика енкодера
    // Encoder_Diagnostics();
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Ініціалізація енкодера AEAT-9922
 *
 * Налаштування:
 * - SPI1: 12.5 MHz, 8-біт, MSB First
 * - CS: PA4
 * - MSEL: PB0 (HIGH для SPI режиму)
 * - Роздільна здатність: 18-біт (262144 позицій)
 * - Інкрементальна: 1024 CPR
 */
void Encoder_AEAT9922_Init(void)
{
    // Конфігурація AEAT-9922
    AEAT9922_Config_t config = {
        // SPI конфігурація
        .spi_config = {
            .spi_handle = &ENCODER_SPI,           // hspi1
            .cs_port = ENCODER_CS_GPIO_PORT,      // GPIOA
            .cs_pin = ENCODER_CS_PIN,             // GPIO_PIN_4
            .timeout_ms = 100
        },

        // MSEL пін (HIGH для SPI4 режиму)
        .msel_port = ENCODER_MSEL_GPIO_PORT,      // GPIOB
        .msel_pin = ENCODER_MSEL_PIN,             // GPIO_PIN_0

        // Роздільна здатність
        .abs_resolution = AEAT9922_ABS_RES_18BIT, // 262144 позицій (макс. точність)
        .incremental_cpr = 1024,                   // 1024 імпульсів/оберт

        // Інтерфейс
        .interface_mode = AEAT9922_INTERFACE_SPI4_16BIT,

        // Напрямок (false = CW count up)
        .direction_ccw = false,

        // Інкрементальний вихід (вимкнено для базового прикладу)
        .enable_incremental = false,
        .encoder_timer_handle = NULL
    };

    // Створити драйвер
    Servo_Status_t status = AEAT9922_Create(&encoder, &config);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to create AEAT-9922 driver (status: %d)\n", status);
        Error_Handler();
    }

    // Ініціалізувати
    status = AEAT9922_Init(&encoder);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to init AEAT-9922 (status: %d)\n", status);
        Error_Handler();
    }

    // Перевірити статус магніту
    AEAT9922_ReadStatus(&encoder);

    if (encoder.status.magnet_high) {
        printf("WARNING: Magnet too close! Increase distance by 0.1-0.3 mm\n");
    }

    if (encoder.status.magnet_low) {
        printf("WARNING: Magnet too far! Decrease distance by 0.1-0.3 mm\n");
    }

    if (encoder.status.memory_error) {
        printf("ERROR: EEPROM memory error! Reprogram configuration\n");
        Error_Handler();
    }

    if (encoder.status.ready) {
        printf("AEAT-9922 encoder initialized successfully!\n");
        printf("Configuration:\n");
        printf("  Resolution: 18-bit (262144 positions)\n");
        printf("  Interface: SPI4 16-bit mode\n");
        printf("  CPR: 1024\n");
    }
}

/**
 * @brief Простий тест зчитування кута
 */
void Encoder_Simple_Test(void)
{
    float angle, velocity;

    // Зчитати поточний кут
    Servo_Status_t status = AEAT9922_ReadAngle(&encoder.interface, &angle);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to read angle (status: %d)\n", status);
        return;
    }

    // Отримати швидкість
    AEAT9922_GetVelocity(&encoder.interface, &velocity);

    // Виведення через UART (потребує налаштований printf)
    printf("Angle: %6.2f deg, Velocity: %7.2f deg/s\n", angle, velocity);

    // Альтернативно: Індикація через LED
    if (angle > 180.0f) {
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);  // ON
    } else {
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);    // OFF
    }
}

/**
 * @brief Безперервне зчитування з детальним виведенням
 */
void Encoder_Continuous_Read(void)
{
    float angle;
    static uint32_t sample_count = 0;

    // Зчитати кут
    if (AEAT9922_ReadAngle(&encoder.interface, &angle) == SERVO_OK) {
        sample_count++;

        // Виводити кожні 100 зразків (~1 секунда при 100 Hz)
        if (sample_count % 100 == 0) {
            printf("\n========== Encoder Status ==========\n");
            printf("Angle:      %.3f degrees\n", encoder.angle_degrees);
            printf("Velocity:   %.2f deg/s\n", encoder.velocity);
            printf("Raw value:  %lu\n", encoder.raw_position);
            printf("Samples:    %lu\n", sample_count);
            printf("Errors:     %lu\n", encoder.error_count);
            printf("Revolutions: %lu\n", encoder.revolution_count);
            printf("====================================\n\n");
        }
    }
}

/**
 * @brief Діагностика енкодера
 */
void Encoder_Diagnostics(void)
{
    printf("\n========== AEAT-9922 Diagnostics ==========\n");

    // Перевірити статус
    Servo_Status_t status = AEAT9922_ReadStatus(&encoder);
    if (status != SERVO_OK) {
        printf("ERROR: Cannot read status register\n");
        return;
    }

    // Статус готовності
    printf("Ready:         %s\n", encoder.status.ready ? "YES" : "NO");

    // Статус магніту
    if (encoder.status.magnet_high) {
        printf("Magnet:        TOO CLOSE (MHI flag)\n");
        printf("Recommendation: Increase gap by 0.1-0.3 mm\n");
        printf("Target distance: 1.0 mm from chip\n");
    } else if (encoder.status.magnet_low) {
        printf("Magnet:        TOO FAR (MLO flag)\n");
        printf("Recommendation: Decrease gap by 0.1-0.3 mm\n");
        printf("Target distance: 1.0 mm from chip\n");
    } else {
        printf("Magnet:        OK (optimal distance)\n");
    }

    // Статус пам'яті
    if (encoder.status.memory_error) {
        printf("EEPROM:        ERROR (MEM_Err flag)\n");
        printf("Recommendation: Reprogram configuration\n");
    } else {
        printf("EEPROM:        OK\n");
    }

    // Лічильник помилок
    printf("Error count:   %lu\n", encoder.error_count);

    // Тест зчитування
    float test_angle;
    status = AEAT9922_ReadAngle(&encoder.interface, &test_angle);
    if (status == SERVO_OK) {
        printf("Test read:     OK (angle: %.2f deg)\n", test_angle);
    } else {
        printf("Test read:     FAILED (status: %d)\n", status);
    }

    printf("===========================================\n\n");
}

/**
 * @brief Приклад калібрування енкодера
 *
 * УВАГА: Цей код виконується ОДИН РАЗ при налаштуванні системи!
 * Не викликайте в циклі!
 */
void Encoder_Calibration_Example(void)
{
    Servo_Status_t status;

    printf("\n========== Encoder Calibration ==========\n");

    // 1. Калібрування точності (вал має обертатися 60-2000 RPM)
    printf("Starting accuracy calibration...\n");
    printf("Ensure motor is rotating at 60-2000 RPM\n");
    printf("Waiting for 2 seconds...\n");

    HAL_Delay(2000);  // Дати час на розгін мотора

    status = AEAT9922_CalibrateAccuracy(&encoder);
    if (status == SERVO_OK) {
        printf("Accuracy calibration: PASS\n");
    } else {
        printf("Accuracy calibration: FAIL\n");
        printf("Check: magnet alignment, rotation speed (60-2000 RPM)\n");
        return;
    }

    HAL_Delay(500);

    // 2. Калібрування нуля (вал має бути нерухомий)
    printf("\nStarting zero calibration...\n");
    printf("Position shaft at desired zero position\n");
    printf("Ensure shaft is stationary\n");
    printf("Press USER button when ready...\n");

    // Зачекати на кнопку (замініть на ваш код)
    // while (HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN) == GPIO_PIN_SET);
    HAL_Delay(3000);  // Або просто затримка для прикладу

    status = AEAT9922_CalibrateZero(&encoder);
    if (status == SERVO_OK) {
        printf("Zero calibration: PASS\n");
    } else {
        printf("Zero calibration: FAIL\n");
        printf("Check: shaft must be completely stationary\n");
        return;
    }

    // 3. Збереження в EEPROM
    printf("\nSaving configuration to EEPROM...\n");
    status = AEAT9922_ProgramEEPROM(&encoder);
    if (status == SERVO_OK) {
        printf("Configuration saved successfully!\n");
    } else {
        printf("Failed to save configuration\n");
        return;
    }

    printf("Calibration complete!\n");
    printf("==========================================\n\n");
}

/* Advanced Examples ---------------------------------------------------------*/

#ifdef USE_ENCODER_INCREMENTAL

/**
 * @brief Приклад з інкрементальним виходом
 *
 * Потребує налаштований TIM2 в Encoder Mode
 */
void Encoder_Incremental_Example(void)
{
    extern TIM_HandleTypeDef htim2;

    // Модифікована конфігурація з інкрементальним виходом
    AEAT9922_Config_t config = {
        // ... (інші параметри як у Encoder_AEAT9922_Init)
        .enable_incremental = true,
        .encoder_timer_handle = &htim2
    };

    AEAT9922_Create(&encoder, &config);
    AEAT9922_Init(&encoder);

    // Основний цикл
    while (1) {
        float angle;

        // Зчитати абсолютний кут
        AEAT9922_ReadAngle(&encoder.interface, &angle);

        // Оновити інкрементальний лічильник
        AEAT9922_UpdateIncrementalCount(&encoder);

        // Вивести обидва значення
        printf("Abs: %6.2f deg, Inc: %8ld counts, Revs: %lu\n",
               angle,
               encoder.incremental_count,
               encoder.revolution_count);

        HAL_Delay(ENCODER_SAMPLE_TIME_MS);
    }
}

/**
 * @brief Callback для Index pulse (PA15 EXTI)
 *
 * Додайте до stm32f4xx_it.c:
 *
 * void EXTI15_10_IRQHandler(void)
 * {
 *     HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
 * }
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_15) {  // Index pulse на PA15
        AEAT9922_IndexPulseCallback(&encoder);
    }
}

#endif // USE_ENCODER_INCREMENTAL

/**
 * @brief Приклад інтеграції з ServoLib
 */
void Encoder_ServoLib_Integration(void)
{
    // Після ініціалізації encoder, можна отримати Sensor Interface
    Sensor_Interface_t* sensor_iface = AEAT9922_GetInterface(&encoder);

    // Тепер цей інтерфейс можна передати в Servo Controller
    // servo.sensor = sensor_iface;

    // Використання через абстрактний інтерфейс
    float angle;
    sensor_iface->read_angle(sensor_iface->driver_data, &angle);
    printf("Angle via interface: %.2f deg\n", angle);
}

/**
 * @brief Приклад з програмною фільтрацією
 */
void Encoder_Filtered_Reading(void)
{
    static float filtered_angle = 0.0f;
    const float alpha = 0.1f;  // Коефіцієнт фільтра (0-1)
    float raw_angle;

    // Зчитати сирий кут
    if (AEAT9922_ReadAngle(&encoder.interface, &raw_angle) == SERVO_OK) {
        // Експоненціальний фільтр
        filtered_angle = alpha * raw_angle + (1.0f - alpha) * filtered_angle;

        printf("Raw: %.2f deg, Filtered: %.2f deg\n", raw_angle, filtered_angle);
    }
}

/**
 * @brief Приклад зчитування з обробкою помилок
 */
void Encoder_Robust_Reading(void)
{
    const int max_retries = 3;
    float angle;
    Servo_Status_t status;

    // Спроба зчитування з повторами
    for (int retry = 0; retry < max_retries; retry++) {
        status = AEAT9922_ReadAngle(&encoder.interface, &angle);

        if (status == SERVO_OK) {
            // Успішно зчитано
            printf("Angle: %.2f deg\n", angle);
            return;
        } else if (status == SERVO_TIMEOUT) {
            printf("WARNING: SPI timeout, retry %d/%d\n", retry + 1, max_retries);
            HAL_Delay(1);
        } else {
            printf("ERROR: SPI error %d, retry %d/%d\n", status, retry + 1, max_retries);
            HAL_Delay(1);
        }
    }

    // Всі спроби невдалі
    printf("ERROR: Failed to read encoder after %d retries\n", max_retries);
    encoder.error_count++;

    // Перевірити статус енкодера
    AEAT9922_ReadStatus(&encoder);
    if (!encoder.status.ready) {
        printf("ERROR: Encoder not ready - check power and connections\n");
    }
}
