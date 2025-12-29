/**
 * @file pwm.c
 * @brief Реалізація PWM драйвера двигуна
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

/* Компілювати цей файл тільки для реального PWM драйвера,
 * для емуляції використовується pwm_udp.c */
#ifdef USE_MOTOR_PWM

#include "../../../Inc/drv/motor/pwm.h"
#include "../../../Inc/config.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Конвертація вказівника на інтерфейс → драйвер
 */
static inline PWM_Motor_Driver_t* GetDriverFromInterface(Motor_Interface_t* iface)
{
    return (PWM_Motor_Driver_t*)iface->driver_data;
}

/**
 * @brief Застосування PWM сигналу для одноканального режиму (PWM + DIR)
 */
static Servo_Status_t ApplySingleChannelPWM(PWM_Motor_Driver_t* driver, float power)
{
    // Абсолютне значення потужності для PWM
    float duty = fabsf(power);

    // Встановлення duty cycle (потужність)
    HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, duty);

    // Встановлення напрямку через GPIO (якщо доступний)
    if (driver->config.gpio_dir != NULL) {
        HWD_GPIO_PinState_t dir_state;

        // Напрямок залежить від знаку потужності
        // power >= 0 → прямий хід (DIR = LOW)
        // power < 0  → зворотний хід (DIR = HIGH)
        if (power >= 0.0f) {
            dir_state = HWD_GPIO_PIN_RESET;  // Прямий хід
        } else {
            dir_state = HWD_GPIO_PIN_SET;    // Зворотний хід
        }

        HWD_GPIO_WritePinDescriptor(&driver->gpio_dir_pin, dir_state);
    }

    return SERVO_OK;
}

/**
 * @brief Застосування PWM сигналу для двоканального режиму (H-bridge)
 */
static Servo_Status_t ApplyDualChannelPWM(PWM_Motor_Driver_t* driver, float power)
{
    if (power > 0.0f) {
        // Прямий хід
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, power);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
        }
    } else if (power < 0.0f) {
        // Зворотний хід
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, fabsf(power));
        }
    } else {
        // Зупинка
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
        }
    }

    return SERVO_OK;
}

/* Hardware callbacks --------------------------------------------------------*/

/**
 * @brief Hardware Init - ініціалізація PWM каналів
 */
static Servo_Status_t PWM_HW_Init(void* driver_data, const Motor_Params_t* params)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Запуск PWM каналів
    if (driver->config.pwm_fwd != NULL) {
        Servo_Status_t status = HWD_PWM_Start(driver->config.pwm_fwd);
        if (status != SERVO_OK) {
            return status;
        }
    }

    if (driver->config.pwm_bwd != NULL) {
        Servo_Status_t status = HWD_PWM_Start(driver->config.pwm_bwd);
        if (status != SERVO_OK) {
            return status;
        }
    }

    driver->current_duty_percent = 0.0f;
    driver->is_braking = false;

    return SERVO_OK;
}

/**
 * @brief Hardware DeInit - деініціалізація PWM каналів
 */
static Servo_Status_t PWM_HW_DeInit(void* driver_data)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Зупинка PWM перед деініціалізацією
    PWM_HW_Stop(driver_data);

    return SERVO_OK;
}

/**
 * @brief Hardware SetPower - встановлення PWM сигналів
 */
static Servo_Status_t PWM_HW_SetPower(void* driver_data, const Motor_Command_t* cmd, float processed_power)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL || cmd == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка типу команди - DC мотор приймає тільки DC команди
    if (cmd->type != MOTOR_TYPE_DC_PWM) {
        return SERVO_INVALID;
    }

    // Застосування PWM відповідно до типу керування
    // processed_power вже оброблена Motor_Base (обмеження, інверсія, мертва зона)
    Servo_Status_t status;
    switch (driver->config.type) {
        case PWM_MOTOR_TYPE_SINGLE_PWM_DIR:
            status = ApplySingleChannelPWM(driver, processed_power);
            break;

        case PWM_MOTOR_TYPE_DUAL_PWM:
            status = ApplyDualChannelPWM(driver, processed_power);
            break;

        default:
            return SERVO_ERROR;
    }

    driver->current_duty_percent = fabsf(processed_power);
    driver->is_braking = false;

    return status;
}

/**
 * @brief Hardware Stop - зупинка PWM каналів
 */
static Servo_Status_t PWM_HW_Stop(void* driver_data)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Встановлення нульової потужності на PWM каналах
    if (driver->config.pwm_fwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
    }

    if (driver->config.pwm_bwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
    }

    driver->current_duty_percent = 0.0f;
    driver->is_braking = false;

    return SERVO_OK;
}

/**
 * @brief Hardware Update - оновлення апаратури (якщо потрібно)
 */
static Servo_Status_t PWM_HW_Update(void* driver_data)
{
    // Для PWM драйвера оновлення апаратури не потрібне
    (void)driver_data;
    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                const PWM_Motor_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    // Очищення структури
    memset(driver, 0, sizeof(PWM_Motor_Driver_t));

    // Копіювання конфігурації
    driver->config = *config;

    // Ініціалізація GPIO для напрямку (якщо використовується PWM + DIR режим)
    if (config->type == PWM_MOTOR_TYPE_SINGLE_PWM_DIR && config->gpio_dir != NULL) {
        driver->gpio_dir_pin.port = config->gpio_dir;
        driver->gpio_dir_pin.pin = (uint16_t)config->gpio_pin;
        driver->gpio_dir_pin.mode = HWD_GPIO_MODE_OUTPUT;
        driver->gpio_dir_pin.pull = HWD_GPIO_NOPULL;

        // Ініціалізація GPIO піна
        Servo_Status_t status = HWD_GPIO_InitPin(&driver->gpio_dir_pin);
        if (status != SERVO_OK) {
            return status;
        }

        // Встановлення початкового стану (прямий хід)
        HWD_GPIO_WritePinDescriptor(&driver->gpio_dir_pin, HWD_GPIO_PIN_RESET);
    }

    // Налаштування hardware callbacks
    driver->interface.hw.init = PWM_HW_Init;
    driver->interface.hw.deinit = PWM_HW_DeInit;
    driver->interface.hw.set_power = PWM_HW_SetPower;
    driver->interface.hw.stop = PWM_HW_Stop;
    driver->interface.hw.update = PWM_HW_Update;

    // Прив'язка driver_data до самого себе
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

#endif /* USE_MOTOR_PWM */
