/**
 * @file board.c
 * @brief Ініціалізація для емуляції на ПК
 * @author ServoCore Team
 * @date 2025
 *
 * Цей файл містить функції ініціалізації для емуляції сервоприводу на ПК.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"
#include "../../Inc/hwd/hwd.h"
#include "../../Inc/hwd/hwd_udp.h"
#include "hwd_timer.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void Board_SystemClock_Config(void);
static void Board_Peripheral_Init(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація емуляційної плати
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Board_Init(void)
{
    // Налаштування системного таймера
    Board_SystemClock_Config();

    // Ініціалізація периферії
    Board_Peripheral_Init();

    // Ініціалізація апаратної абстракції
    Servo_Status_t status = HWD_Init();
    if (status != SERVO_OK) {
        return status;
    }

    return SERVO_OK;
}

/**
 * @brief Деініціалізація емуляційної плати
 *
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Board_DeInit(void)
{
    // Деініціалізація апаратної абстракції
    Servo_Status_t status = HWD_DeInit();
    if (status != SERVO_OK) {
        return status;
    }

    return SERVO_OK;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Налаштування системного таймера для емуляції
 */
static void Board_SystemClock_Config(void)
{
    // Ініціалізація таймера для емуляції
    HWD_Timer_Init();
    printf("System clock configured for emulation at %d Hz\n", SYSTEM_TICK_FREQ_HZ);
}

/**
 * @brief Ініціалізація емуляційної периферії
 */
static void Board_Peripheral_Init(void)
{
    // Для емуляції на ПК ініціалізуємо UDP зв'язок
    Servo_Status_t status = HWD_UDP_Init(UDP_SERVER_IP, UDP_SERVER_PORT, UDP_CLIENT_PORT);
    if (status != SERVO_OK) {
        printf("ERROR: Failed to initialize UDP connection\n");
        return;
    }

    printf("Emulation board peripherals initialized\n");
    printf("UDP Connection: %s:%d -> %s:%d\n",
           UDP_SERVER_IP, UDP_SERVER_PORT, "localhost", UDP_CLIENT_PORT);
}