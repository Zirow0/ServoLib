/**
 * @file pwm.c
 * @brief Реалізація PWM драйвера DC двигуна
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_MOTOR_PWM

#include "../../../Inc/drv/motor/pwm.h"
#include <string.h>
#include <math.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief PWM + DIR режим: абсолютне значення на PWM, знак на GPIO
 */
static Servo_Status_t ApplySingleChannelPWM(PWM_Motor_Driver_t* driver, float power)
{
    HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, fabsf(power));

    if (driver->config.gpio_dir != NULL) {
        HWD_GPIO_PinState_t dir = (power >= 0.0f) ? HWD_GPIO_PIN_RESET : HWD_GPIO_PIN_SET;
        HWD_GPIO_WritePin(driver->gpio_dir.port, driver->gpio_dir.pin, dir);
    }

    return SERVO_OK;
}

/**
 * @brief H-bridge режим: позитивна потужність на fwd, негативна на bwd
 */
static Servo_Status_t ApplyDualChannelPWM(PWM_Motor_Driver_t* driver, float power)
{
    if (power > 0.0f) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, power);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
        }
    } else if (power < 0.0f) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, fabsf(power));
        }
    } else {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
        if (driver->config.pwm_bwd != NULL) {
            HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
        }
    }

    return SERVO_OK;
}

/* Hardware callbacks --------------------------------------------------------*/

static Servo_Status_t PWM_HW_Init(void* driver_data, const Motor_Params_t* params)
{
    (void)params;
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    if (driver->config.pwm_fwd != NULL) {
        Servo_Status_t s = HWD_PWM_Start(driver->config.pwm_fwd);
        if (s != SERVO_OK) return s;
    }

    if (driver->config.pwm_bwd != NULL) {
        Servo_Status_t s = HWD_PWM_Start(driver->config.pwm_bwd);
        if (s != SERVO_OK) return s;
    }

    return SERVO_OK;
}

static Servo_Status_t PWM_HW_SetPower(void* driver_data, float processed_power)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    switch (driver->config.type) {
        case PWM_MOTOR_TYPE_SINGLE_PWM_DIR:
            return ApplySingleChannelPWM(driver, processed_power);
        case PWM_MOTOR_TYPE_DUAL_PWM:
            return ApplyDualChannelPWM(driver, processed_power);
        default:
            return SERVO_ERROR;
    }
}

static Servo_Status_t PWM_HW_Stop(void* driver_data)
{
    PWM_Motor_Driver_t* driver = (PWM_Motor_Driver_t*)driver_data;
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    if (driver->config.pwm_fwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_fwd, 0.0f);
    }

    if (driver->config.pwm_bwd != NULL) {
        HWD_PWM_SetDutyPercent(driver->config.pwm_bwd, 0.0f);
    }

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t PWM_Motor_Create(PWM_Motor_Driver_t* driver,
                                const PWM_Motor_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    memset(driver, 0, sizeof(PWM_Motor_Driver_t));

    driver->config = *config;

    /* Ініціалізація GPIO для режиму PWM + DIR */
    if (config->type == PWM_MOTOR_TYPE_SINGLE_PWM_DIR && config->gpio_dir != NULL) {
        driver->gpio_dir.port = config->gpio_dir;
        driver->gpio_dir.pin  = (uint16_t)config->gpio_pin;
        driver->gpio_dir.mode = HWD_GPIO_MODE_OUTPUT;
        driver->gpio_dir.pull = HWD_GPIO_NOPULL;

        Servo_Status_t s = HWD_GPIO_InitPin(&driver->gpio_dir);
        if (s != SERVO_OK) return s;

        HWD_GPIO_WritePin(driver->gpio_dir.port, driver->gpio_dir.pin, HWD_GPIO_PIN_RESET);
    }

    /* Hardware callbacks */
    driver->interface.hw.init      = PWM_HW_Init;
    driver->interface.hw.set_power = PWM_HW_SetPower;
    driver->interface.hw.stop      = PWM_HW_Stop;

    driver->interface.driver_data = driver;

    return SERVO_OK;
}

#endif /* USE_MOTOR_PWM */
