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

#ifdef _WIN32
    #include <windows.h>
#endif

#include "../Inc/core.h"
#include "../Inc/config.h"
#include "../Board/PC_Emulation/board_config.h"
#include "../Inc/ctrl/servo.h"
#include "../Inc/drv/motor/pwm_udp.h"
#include "../Inc/drv/brake/brake_udp.h"
#include "../Inc/drv/sensor/sensor_udp.h"
#include "udp_client.h"

/* Private defines -----------------------------------------------------------*/
#define SERVO_UPDATE_PERIOD_MS    5    // 200 Hz
#define MAX_SIMULATION_TIME_S     60   // 60 секунд симуляції

/* Private variables ---------------------------------------------------------*/
static Servo_Controller_t servo_controller;
static PWM_Motor_UDP_Driver_t motor_driver;
static Brake_UDP_Driver_t brake_driver;
static Sensor_UDP_Driver_t sensor_driver;

static uint32_t start_time_ms = 0;
static uint32_t last_update_ms = 0;

/* Private function prototypes -----------------------------------------------*/
static uint32_t GetTickCountCustom(void);
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
static uint32_t GetTickCountCustom(void)
{
    #ifdef _WIN32
        // Windows implementation using GetTickCount
        return (uint32_t)GetTickCount();
    #else
        // POSIX implementation using clock_gettime
        static time_t start_time = 0;
        struct timespec ts;

        clock_gettime(CLOCK_MONOTONIC, &ts);

        if (start_time == 0) {
            start_time = ts.tv_sec;
        }

        return (uint32_t)((ts.tv_sec - start_time) * 1000 + ts.tv_nsec / 10000);
    #endif
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
            .Kp = 1.0f,
            .Ki = 0.1f,
            .Kd = 0.01f,
            .out_min = -100.0f,
            .out_max = 10.0f,
            .sample_time = 0.001f,  // 1ms sample time
            .direction = PID_DIRECTION_DIRECT
        },
        .safety_config = {
            .min_position = 0.0f,
            .max_position = 360.0f,
            .enable_position_limits = true,
            .max_velocity = 200.0f,
            .enable_velocity_limit = true,
            .max_acceleration = 360.0f,
            .enable_acceleration_limit = true,
            .max_current = 2000,
            .current_timeout_ms = 100,
            .enable_current_protection = true,
            .watchdog_timeout_ms = 1000,
            .enable_watchdog = true,
            .max_temperature = 85,
            .enable_thermal_protection = false
        },
        .traj_params = {
            .max_velocity = 180.0f,
            .max_acceleration = 360.0f,
            .max_jerk = 1000.0f
        },
        .update_frequency = 200.0f,  // 200 Гц
        .enable_brake = true
    };

    // Важливо: Оновлюємо драйвери перед ініціалізацією сервоприводу
    // Це потрібно для отримання початкових даних
    Sensor_UDP_Update(&sensor_driver);
    Brake_UDP_Update(&brake_driver);
    PWM_Motor_UDP_Update(&motor_driver);

    // Ініціалізація сервоприводу з усіма компонентами
    // For the emulator, we can pass NULL for the brake driver since we handle brake control differently
    status = Servo_InitWithBrake(&servo_controller, &servo_config,
                                 (Motor_Interface_t*)&motor_driver,
                                 NULL);  // For emulator, we don't use the traditional brake interface
    if (status != SERVO_OK) {
        printf("ERROR: Servo controller initialization failed (status: %d)\n", status);
        exit(1);
    }

    // Встановлення тестової цілі
    Servo_SetPosition(&servo_controller, 180.0f);

    start_time_ms = GetTickCountCustom();
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
    static uint32_t heartbeat_counter = 0;  // Лічильник для heartbeat
    const uint32_t HEARTBEAT_INTERVAL = 20;  // Відправляти кожні 200 циклів (1 секунда при 200Hz)
    
    // Змінні для управління обертанням
    static uint32_t rotation_start_time = 0;
    static int rotation_state = 0; // 0 - початкова позиція, 1 - повернуто на +90, 2 - очікування після +90, 3 - повернуто на -90, 4 - очікування після -90
    const uint32_t WAIT_TIME_MS = 5000; // 5 секунд очікування
    
    // Встановлюємо початкову позицію
    Servo_SetPosition(&servo_controller, 0.0f);
    rotation_start_time = GetTickCountCustom();

    while (simulation_time_s < MAX_SIMULATION_TIME_S) {
        current_time = GetTickCountCustom();

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

            // Алгоритм обертання двигуна
            float target_position = Servo_GetPosition(&servo_controller);
            
            switch (rotation_state) {
                case 0: // Початкова позиція (0 градусів)
                    target_position = 0.0f;
                    if (current_time - rotation_start_time >= 1000) { // Зачекати 1 секунду перед першим обертанням
                        Servo_SetPosition(&servo_controller, 90.0f);
                        rotation_start_time = current_time;
                        rotation_state = 1;
                        printf("Rotating to +90 degrees\n");
                    }
                    break;
                    
                case 1: // Повернуто на +90
                    target_position = 90.0f;
                    if (current_time - rotation_start_time >= 1000) { // Дозволити час на досягнення позиції
                        rotation_state = 2;
                        rotation_start_time = current_time;
                        printf("Reached +90 degrees, waiting 5 seconds\n");
                    }
                    break;
                    
                case 2: // Очікування після +90
                    target_position = 90.0f;
                    if (current_time - rotation_start_time >= WAIT_TIME_MS) { // Очікування 5 секунд
                        Servo_SetPosition(&servo_controller, -90.0f);
                        rotation_start_time = current_time;
                        rotation_state = 3;
                        printf("Rotating to -90 degrees\n");
                    }
                    break;
                    
                case 3: // Повернуто на -90
                    target_position = -90.0f;
                    if (current_time - rotation_start_time >= 1000) { // Дозволити час на досягнення позиції
                        rotation_state = 4;
                        rotation_start_time = current_time;
                        printf("Reached -90 degrees, waiting 5 seconds\n");
                    }
                    break;
                    
                case 4: // Очікування після -90
                    target_position = -90.0f;
                    if (current_time - rotation_start_time >= WAIT_TIME_MS) { // Очікування 5 секунд
                        Servo_SetPosition(&servo_controller, 0.0f);
                        rotation_start_time = current_time;
                        rotation_state = 0;
                        printf("Returning to 0 degrees, restarting cycle\n");
                    }
                    break;
            }

            // Відправка heartbeat кожну секунду
            heartbeat_counter++;
            if (heartbeat_counter >= HEARTBEAT_INTERVAL) {
                Servo_Status_t ping_status = UDP_Client_Ping();
                if (ping_status != SERVO_OK) {
                    // Не виводимо помилку для тестування без мережі
                    // printf("WARNING: Heartbeat ping failed (status: %d)\n", ping_status);
                }
                heartbeat_counter = 0;
            }

            // Оновлення сервоприводу
            status = Servo_Update(&servo_controller);
            if (status != SERVO_OK) {
                printf("WARNING: Servo update failed (status: %d)\n", status);
            }

            // Виведення даних кожні 100 ітерацій (кожну 100 мс при 20 Гц)
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
        #ifdef _WIN32
            Sleep(1);  // Sleep for 1ms on Windows
        #else
            struct timespec ts = {0, 1000000};  // 1 millisecond (1,000,000 ns)
            nanosleep(&ts, NULL);
        #endif
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