/**
 * @file main.c
 * @brief Головний файл для емуляції сервоприводу
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить головний цикл емуляції сервоприводу на ПК.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "../Inc/core.h"
#include "../Inc/config.h"
#include "../Board/PC_Emulation/board.h"
#include "../Inc/ctrl/servo.h"
#include "../Inc/drv/motor/pwm_udp.h"
#include "../Inc/drv/brake/brake_udp.h"
#include "../Inc/drv/sensor/sensor_udp.h"
#include "udp_client.h"

/* Private defines -----------------------------------------------------------*/
#define SERVO_UPDATE_PERIOD_MS    1    // 1000 Hz
#define MAX_SIMULATION_TIME_S     60   // 60 секунд симуляції

/* Private variables ---------------------------------------------------------*/
static Servo_Controller_t servo_controller;
static PWM_Motor_UDP_Driver_t motor_driver;
static Brake_UDP_Driver_t brake_driver;
static Sensor_UDP_Driver_t sensor_driver;

static uint32_t start_time_ms = 0;
static uint32_t last_update_ms = 0;

/* Private function prototypes -----------------------------------------------*/
static uint32_t GetTickCount(void);
static void InitializeSystem(void);
static void RunMainLoop(void);
static void CleanupSystem(void);

/* Main functions ------------------------------------------------------------*/

/**
 * @brief Головна функція застосунку емуляції
 */
int main(void)
{
    printf("ServoLib PC Emulation System Starting...\n");

    // Ініціалізація системи
    InitializeSystem();

    // Запуск головного циклу
    RunMainLoop();

    // Очищення ресурсів
    CleanupSystem();

    printf("Emulation completed\n");
    return 0;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Отримання поточного часу в мілісекундах
 *
 * @return uint32_t Час в мілісекундах від початку епохи
 */
static uint32_t GetTickCount(void)
{
    static time_t start_time = 0;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (start_time == 0) {
        start_time = ts.tv_sec;
    }

    return (uint32_t)((ts.tv_sec - start_time) * 1000 + ts.tv_nsec / 1000000);
}

/**
 * @brief Ініціалізація системи емуляції
 */
static void InitializeSystem(void)
{
    printf("Initializing emulation system...\n");

    // Ініціалізація емуляційної плати
    Servo_Status_t status = Board_Init();
    if (status != SERVO_OK) {
        printf("ERROR: Board initialization failed (status: %d)\n", status);
        exit(1);
    }

    // Ініціалізація драйверів
    // 1. Сенсор (найперший)
    status = Sensor_UDP_Create(&sensor_driver);
    if (status != SERVO_OK) {
        printf("ERROR: Sensor driver creation failed (status: %d)\n", status);
        exit(1);
    }

    status = Sensor_UDP_Init(&sensor_driver);
    if (status != SERVO_OK) {
        printf("ERROR: Sensor driver initialization failed (status: %d)\n", status);
        exit(1);
    }

    // 2. Двигун
    PWM_Motor_UDP_Config_t motor_config = {
        .type = MOTOR_TYPE_DC_PWM,
        .interface_mode = PWM_MOTOR_INTERFACE_UDP
    };

    status = PWM_Motor_UDP_Create(&motor_driver, &motor_config);
    if (status != SERVO_OK) {
        printf("ERROR: Motor driver creation failed (status: %d)\n", status);
        exit(1);
    }

    Motor_Params_t motor_params = {
        .max_power = 100.0f,
        .min_power = 5.0f,
        .max_current = 2000,
        .invert_direction = false
    };

    status = PWM_Motor_UDP_Init(&motor_driver, &motor_params);
    if (status != SERVO_OK) {
        printf("ERROR: Motor driver initialization failed (status: %d)\n", status);
        exit(1);
    }

    // 3. Гальма
    Brake_UDP_Config_t brake_config = {
        .timeout_ms = 100,
        .update_interval_ms = 10,
        .auto_engage_on_error = true,
        .error_threshold = 10
    };

    status = Brake_UDP_Create(&brake_driver, &brake_config);
    if (status != SERVO_OK) {
        printf("ERROR: Brake driver creation failed (status: %d)\n", status);
        exit(1);
    }

    status = Brake_UDP_Init(&brake_driver);
    if (status != SERVO_OK) {
        printf("ERROR: Brake driver initialization failed (status: %d)\n", status);
        exit(1);
    }

    // Ініціалізація сервоприводу
    Servo_Config_t servo_config = {
        .axis_config = {
            .motor_type = MOTOR_TYPE_DC_PWM,
            .sensor_type = SENSOR_TYPE_ENCODER_MAG,
            .max_velocity = 180.0f,      // 180 град/с
            .max_acceleration = 360.0f,  // 360 град/с²
            .max_current = 2000.0f,      // 2A
            .position_min = 0.0f,
            .position_max = 360.0f,
            .enable_limits = true,
            .invert_direction = false
        },
        .pid_params = {
            .kp = 1.0f,
            .ki = 0.1f,
            .kd = 0.01f,
            .output_min = -100.0f,
            .output_max = 100.0f
        },
        .safety_config = {
            .position_limits_enabled = true,
            .velocity_limit = 200.0f,
            .current_limit = 2000.0f,
            .watchdog_timeout_ms = 1000
        },
        .traj_params = {
            .max_velocity = 180.0f,
            .max_acceleration = 360.0f,
            .max_jerk = 1000.0f
        },
        .update_frequency = 1000.0f,  // 1 кГц
        .enable_brake = true
    };

    // Важливо: Оновлюємо драйвери перед ініціалізацією сервоприводу
    // Це потрібно для отримання початкових даних
    Sensor_UDP_Update(&sensor_driver);
    Brake_UDP_Update(&brake_driver);
    PWM_Motor_UDP_Update(&motor_driver);

    // Для повної емуляції створимо інтерфейси для сервоприводу
    // Потрібно створити тимчасові інтерфейси або використовувати функції для отримання інтерфейсів
    Servo_Interface_t servo_interface;

    // Ініціалізація сервоприводу з усіма компонентами
    status = Servo_InitWithBrake(&servo_controller, &servo_config,
                                 &motor_driver.interface,
                                 &brake_driver.driver);
    if (status != SERVO_OK) {
        printf("ERROR: Servo controller initialization failed (status: %d)\n", status);
        exit(1);
    }

    // Встановлення тестової цілі
    Servo_SetPosition(&servo_controller, 180.0f);

    start_time_ms = GetTickCount();
    last_update_ms = start_time_ms;

    printf("System initialized successfully\n");
}

/**
 * @brief Головний цикл емуляції
 */
static void RunMainLoop(void)
{
    printf("Starting main emulation loop...\n");

    uint32_t current_time;
    uint32_t simulation_time_s = 0;

    while (simulation_time_s < MAX_SIMULATION_TIME_S) {
        current_time = GetTickCount();

        // Перевірка періоду оновлення
        if (current_time - last_update_ms >= SERVO_UPDATE_PERIOD_MS) {
            last_update_ms = current_time;

            // Оновлення драйверів перед оновленням сервоприводу
            Servo_Status_t status = Sensor_UDP_Update(&sensor_driver);
            if (status != SERVO_OK) {
                printf("WARNING: Sensor update failed (status: %d)\n", status);
            }

            status = PWM_Motor_UDP_Update(&motor_driver);
            if (status != SERVO_OK) {
                printf("WARNING: Motor update failed (status: %d)\n", status);
            }

            status = Brake_UDP_Update(&brake_driver);
            if (status != SERVO_OK) {
                printf("WARNING: Brake update failed (status: %d)\n", status);
            }

            // Оновлення сервоприводу
            status = Servo_Update(&servo_controller);
            if (status != SERVO_OK) {
                printf("WARNING: Servo update failed (status: %d)\n", status);
            }

            // Виведення даних кожні 100 ітерацій (кожну 100 мс при 1 кГц)
            if ((current_time / 100) > (start_time_ms / 100)) {
                float position = Servo_GetPosition(&servo_controller);
                float velocity = Servo_GetVelocity(&servo_controller);
                Servo_State_t state = Servo_GetState(&servo_controller);

                printf("Time: %u ms, Pos: %.2f deg, Vel: %.2f deg/s, State: %d\n",
                       current_time - start_time_ms,
                       position,
                       velocity,
                       state);
            }
        }

        // Оновлення часу симуляції
        simulation_time_s = (current_time - start_time_ms) / 1000;

        // Мала затримка для зменшення використання CPU
        struct timespec ts = {0, 100000};  // 100 мікросекунд
        nanosleep(&ts, NULL);
    }

    printf("Main loop completed after %u seconds\n", simulation_time_s);
}

/**
 * @brief Очищення ресурсів системи
 */
static void CleanupSystem(void)
{
    printf("Cleaning up system...\n");

    // Зупинка сервоприводу
    Servo_Stop(&servo_controller);

    // Завершення UDP клієнта
    UDP_Client_DeInit();

    // Деініціалізація плати
    Board_DeInit();

    printf("System cleanup completed\n");
}