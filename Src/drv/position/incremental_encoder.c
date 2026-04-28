/**
 * @file incremental_encoder.c
 * @brief Реалізація драйвера інкрементального квадратурного енкодера
 *
 * Позиція: EXTI X4 state machine → volatile int32_t count
 * Швидкість: TIM Input Capture → volatile uint32_t period_us
 *
 * Board ISR читає GPIO і викликає Incremental_Encoder_EXTI_Handler.
 * Board TIM IC ISR обчислює period_us і викликає Incremental_Encoder_IC_Handler.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_INCREMENTAL

#include "../../../Inc/drv/position/incremental_encoder.h"
#include "../../../Inc/hwd/hwd_timer.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/** Таймаут відсутності імпульсів → швидкість = 0 */
#define ENC_SPEED_TIMEOUT_MS  500u

/* Private data --------------------------------------------------------------*/

/**
 * X4 state machine lookup table.
 * Індекс = (старий_стан << 2) | новий_стан, де стан = (B << 1) | A
 *  0 → немає руху, +1 → вперед, -1 → назад, 2 → помилка (пропущено)
 */
static const int8_t enc_table[16] = {
/*  нов: 00   01   10   11    стар */
          0,  -1,  +1,   0,  /* 00 */
         +1,   0,   0,  -1,  /* 01 */
         -1,   0,   0,  +1,  /* 10 */
          0,  +1,  -1,   0   /* 11 */
};

/* Private hardware callbacks ------------------------------------------------*/

static Servo_Status_t IncEnc_HW_Init(void* driver_data)
{
    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;

    drv->count          = 0;
    drv->period_us      = 0;
    drv->last_pulse_ms  = 0;
    drv->direction      = 1;
    drv->enc_state      = 0;

    return SERVO_OK;
}

static Servo_Status_t IncEnc_HW_ReadRaw(void* driver_data, Position_Raw_Data_t* raw)
{
    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;
    const int32_t cpr = (int32_t)drv->cpr;

    /* Читання volatile — атомарне для 32-bit на Cortex-M4 */
    int32_t  count      = drv->count;
    uint32_t period_us  = drv->period_us;
    uint32_t last_ms    = drv->last_pulse_ms;
    int8_t   dir        = drv->direction;

    /* Нормалізація лічильника до [0, CPR) */
    int32_t normalized = count % cpr;
    if (normalized < 0) normalized += cpr;

    raw->angle_rad    = (float)normalized / (float)cpr * TWO_PI;
    raw->timestamp_us = HWD_Timer_GetMicros();
    raw->valid        = true;

    /* Швидкість з IC timer */
    if (period_us == 0u ||
        (HWD_Timer_GetMillis() - last_ms) > ENC_SPEED_TIMEOUT_MS) {
        raw->has_velocity    = true;
        raw->velocity_rad_s  = 0.0f;
    } else {
        /* один тік каналу A = 1/CPR оберту → TWO_PI/CPR радіанів */
        float vel = TWO_PI / ((float)period_us * 1e-6f * (float)drv->cpr);
        raw->has_velocity   = true;
        raw->velocity_rad_s = (dir >= 0) ? vel : -vel;
    }

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t* driver,
                                           uint32_t cpr)
{
    if (driver == NULL || cpr == 0u) {
        return SERVO_INVALID;
    }

    memset(driver, 0, sizeof(Incremental_Encoder_Driver_t));

    driver->cpr = cpr;

    driver->interface.hw.init     = IncEnc_HW_Init;
    driver->interface.hw.read_raw = IncEnc_HW_ReadRaw;
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

void Incremental_Encoder_EXTI_Handler(Incremental_Encoder_Driver_t* driver,
                                       uint8_t pin_a, uint8_t pin_b)
{
    uint8_t curr = (uint8_t)((pin_b << 1) | pin_a);
    uint8_t idx  = (uint8_t)((driver->enc_state << 2) | curr);
    int8_t  step = enc_table[idx];

    driver->count     += step;
    driver->enc_state  = curr;

    if (step != 0) {
        driver->direction = step;
    }
}

void Incremental_Encoder_IC_Handler(Incremental_Encoder_Driver_t* driver,
                                     uint32_t period_us)
{
    driver->period_us     = period_us;
    driver->last_pulse_ms = HWD_Timer_GetMillis();
}

#endif /* USE_SENSOR_INCREMENTAL */
