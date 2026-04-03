/**
 * @file incremental_encoder.c
 * @brief Реалізація драйвера інкрементального квадратурного енкодера
 * @author ServoCore Team
 * @date 2025
 *
 * Надає hardware callbacks для Position_Sensor_Interface_t.
 * Використовує HWD_Timer у режимі квадратурного лічильника (x4).
 * Апаратний таймер рахує всі 4 фронти сигналів A і B — вища точність
 * без навантаження на CPU.
 */

/* Includes ------------------------------------------------------------------*/
#include "drv/position/incremental_encoder.h"
#include "hwd/hwd_timer.h"
#include <stddef.h>

/* Private defines -----------------------------------------------------------*/
#define TWO_PI_F    6.28318530717958647f

/* Private functions ---------------------------------------------------------*/

static Servo_Status_t IncEncoder_HW_Init(void* driver_data,
                                          const Position_Params_t* params)
{
    (void)params;

    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;

    return HWD_Timer_EncoderInit(&drv->handle, &drv->config.hw);
}

static Servo_Status_t IncEncoder_HW_DeInit(void* driver_data)
{
    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;

    return HWD_Timer_EncoderDeInit(&drv->handle);
}

/**
 * @brief Зчитування сирих даних з апаратного таймера-енкодера
 *
 * Читає знаковий лічильник, нормалізує до [0, CPR) та перетворює
 * в кут 0-2π. Обробка velocity/multi-turn відбувається в position.c.
 */
static Servo_Status_t IncEncoder_HW_ReadRaw(void* driver_data,
                                              Position_Raw_Data_t* raw)
{
    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;
    const int32_t cpr = (int32_t)drv->config.cpr;

    int32_t count = HWD_Timer_EncoderRead(&drv->handle);

    /* Нормалізація до [0, CPR) з урахуванням від'ємних значень */
    int32_t normalized = count % cpr;
    if (normalized < 0) {
        normalized += cpr;
    }

    raw->angle_rad      = (float)normalized / (float)cpr * TWO_PI_F;
    raw->timestamp_us   = HWD_Timer_GetMicros();
    raw->has_velocity   = false;
    raw->valid          = true;

    return SERVO_OK;
}

/**
 * @brief Калібрування: скидання лічильника до 0 (встановлення нульової позиції)
 */
static Servo_Status_t IncEncoder_HW_Calibrate(void* driver_data)
{
    Incremental_Encoder_Driver_t* drv = (Incremental_Encoder_Driver_t*)driver_data;

    HWD_Timer_EncoderReset(&drv->handle);

    return SERVO_OK;
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Incremental_Encoder_Create(Incremental_Encoder_Driver_t* driver,
                                           const Incremental_Encoder_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    driver->config = *config;

    /* Прив'язка hardware callbacks */
    driver->interface.hw.init        = IncEncoder_HW_Init;
    driver->interface.hw.deinit      = IncEncoder_HW_DeInit;
    driver->interface.hw.read_raw    = IncEncoder_HW_ReadRaw;
    driver->interface.hw.calibrate   = IncEncoder_HW_Calibrate;
    driver->interface.hw.notify_callback = NULL;

    /* Можливості інкрементального енкодера */
    driver->interface.capabilities     = POSITION_CAP_INCREMENTAL
                                       | POSITION_CAP_MULTITURN;
    driver->interface.requires_calibration = true;

    /* Вказівник на конкретний драйвер для callbacks */
    driver->interface.driver_data = driver;

    return SERVO_OK;
}
