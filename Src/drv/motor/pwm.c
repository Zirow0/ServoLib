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

/* Private function prototypes -----------------------------------------------*/
static Servo_Status_t PWM_Motor_Init_Internal(Motor_Interface_t* self,
                                               const Motor_Params_t* params);
static Servo_Status_t PWM_Motor_DeInit_Internal(Motor_Interface_t* self);
static Servo_Status_t PWM_Motor_SetCommand_Internal(Motor_Interface_t* self,
                                                     const Motor_Command_t* cmd);
static Servo_Status_t PWM_Motor_Stop_Internal(Motor_Interface_t* self);
static Servo_Status_t PWM_Motor_EmergencyStop_Internal(Motor_Interface_t* self);
static Servo_Status_t PWM_Motor_GetState_Internal(Motor_Interface_t* self,
                                                   Motor_State_t* state);
static Servo_Status_t PWM_Motor_GetStats_Internal(Motor_Interface_t* self,
                                                   Motor_Stats_t* stats);
static Servo_Status_t PWM_Motor_Update_Internal(Motor_Interface_t* self);

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Конвертація драйвера з інтерфейсу
 */
static inline PWM_Motor_Driver_t* GetDriver(Motor_Interface_t* self)
{
    return (PWM_Motor_Driver_t*)self->driver_data;
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

/* Interface implementation --------------------------------------------------*/

static Servo_Status_t PWM_Motor_Init_Internal(Motor_Interface_t* self,
                                               const Motor_Params_t* params)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Ініціалізація базового драйвера
    Servo_Status_t status = Motor_Base_Init(&driver->base, params);
    if (status != SERVO_OK) {
        return status;
    }

    // Запуск PWM каналів
    if (driver->config.pwm_fwd != NULL) {
        status = HWD_PWM_Start(driver->config.pwm_fwd);
        if (status != SERVO_OK) {
            return status;
        }
    }

    if (driver->config.pwm_bwd != NULL) {
        status = HWD_PWM_Start(driver->config.pwm_bwd);
        if (status != SERVO_OK) {
            return status;
        }
    }

    driver->current_duty_percent = 0.0f;
    driver->is_braking = false;

    return SERVO_OK;
}

static Servo_Status_t PWM_Motor_DeInit_Internal(Motor_Interface_t* self)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Зупинка PWM
    PWM_Motor_Stop_Internal(self);

    // Деініціалізація базового драйвера
    return Motor_Base_DeInit(&driver->base);
}

static Servo_Status_t PWM_Motor_SetCommand_Internal(Motor_Interface_t* self,
                                                     const Motor_Command_t* cmd)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL || cmd == NULL) {
        return SERVO_INVALID;
    }

    // Перевірка типу команди - DC мотор приймає тільки DC команди
    if (cmd->type != MOTOR_TYPE_DC_PWM) {
        return SERVO_INVALID;
    }

    float power = cmd->data.dc.power;

    // Оновлення базової логіки
    Servo_Status_t status = Motor_Base_SetPower(&driver->base, power);
    if (status != SERVO_OK) {
        return status;
    }

    // Застосування PWM відповідно до типу керування
    switch (driver->config.type) {
        case PWM_MOTOR_TYPE_SINGLE_PWM_DIR:
            status = ApplySingleChannelPWM(driver, driver->base.current_power);
            break;

        case PWM_MOTOR_TYPE_DUAL_PWM:
            status = ApplyDualChannelPWM(driver, driver->base.current_power);
            break;

        default:
            return SERVO_ERROR;
    }

    driver->current_duty_percent = fabsf(driver->base.current_power);
    driver->is_braking = false;

    return status;
}

static Servo_Status_t PWM_Motor_Stop_Internal(Motor_Interface_t* self)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Встановлення нульової потужності
    if (driver->config.pwm_fwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
    }

    if (driver->config.pwm_bwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
    }

    driver->current_duty_percent = 0.0f;
    driver->is_braking = false;

    // Оновлення базового стану
    Motor_Base_SetState(&driver->base, MOTOR_STATE_IDLE);
    driver->base.current_power = 0.0f;

    return SERVO_OK;
}

static Servo_Status_t PWM_Motor_EmergencyStop_Internal(Motor_Interface_t* self)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Миттєва зупинка всіх каналів
    PWM_Motor_Stop_Internal(self);

    // Встановлення аварійного прапорця
    driver->base.emergency_flag = true;
    Motor_Base_SetState(&driver->base, MOTOR_STATE_ERROR);

    return SERVO_OK;
}

static Servo_Status_t PWM_Motor_GetState_Internal(Motor_Interface_t* self,
                                                   Motor_State_t* state)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL || state == NULL) {
        return SERVO_INVALID;
    }

    *state = driver->base.state;
    return SERVO_OK;
}

static Servo_Status_t PWM_Motor_GetStats_Internal(Motor_Interface_t* self,
                                                   Motor_Stats_t* stats)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    return Motor_Base_GetStats(&driver->base, stats);
}

static Servo_Status_t PWM_Motor_Update_Internal(Motor_Interface_t* self)
{
    PWM_Motor_Driver_t* driver = GetDriver(self);
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    return Motor_Base_Update(&driver->base);
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

    // Налаштування інтерфейсу
    driver->interface.init = PWM_Motor_Init_Internal;
    driver->interface.deinit = PWM_Motor_DeInit_Internal;
    driver->interface.set_command = PWM_Motor_SetCommand_Internal;
    driver->interface.stop = PWM_Motor_Stop_Internal;
    driver->interface.emergency_stop = PWM_Motor_EmergencyStop_Internal;
    driver->interface.get_state = PWM_Motor_GetState_Internal;
    driver->interface.get_stats = PWM_Motor_GetStats_Internal;
    driver->interface.update = PWM_Motor_Update_Internal;
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

Servo_Status_t PWM_Motor_SetPower(PWM_Motor_Driver_t* driver, float power)
{
    return Motor_SetPower_DC(&driver->interface, power);
}

Motor_Interface_t* PWM_Motor_GetInterface(PWM_Motor_Driver_t* driver)
{
    if (driver == NULL) {
        return NULL;
    }
    return &driver->interface;
}

#endif /* USE_MOTOR_PWM */
