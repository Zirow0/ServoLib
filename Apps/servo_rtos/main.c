/* ============================================================================
 * servo_rtos — одна вісь з FreeRTOS
 *
 * Задачі:
 *   task_current  (5 кГц) — пробуджується TIM2 IRQ → xTaskNotifyFromISR
 *   task_control  (1 кГц) — vTaskDelayUntil, PID + comm RX
 *   task_telem  (100 Гц) — vTaskDelayUntil, телеметрія
 *
 * Спільний стан між задачами: volatile float (один регістр FPU — атомарно
 * на Cortex-M4). Запис лише з task_current, читання лише з task_control.
 * ============================================================================ */

#include "FreeRTOS.h"
#include "task.h"

#include "board_config.h"
#include "ctrl/cascade.h"
#include "ctrl/time.h"
#include "drv/motor/pwm.h"
#include "drv/position/incremental_encoder.h"
#include "drv/brake/gpio_brake.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_gpio.h"
#include "comm/servo_comm.h"
#include "hwd_uart_async.h"

#include <math.h>

/* ── UART async ────────────────────────────────────────────────────────────── */
static const uart_hw_config_t comm_hw = {
    .usart      = COMM_USART,
    .tx_port    = COMM_GPIO_PORT,  .tx_pin    = COMM_TX_PIN,
    .rx_port    = COMM_GPIO_PORT,  .rx_pin    = COMM_RX_PIN,
    .rx_dma     = COMM_RX_DMA,
    .rx_stream  = COMM_RX_DMA_STREAM,
    .rx_channel = COMM_RX_DMA_CHANNEL,
    .tx_dma     = COMM_TX_DMA,
    .tx_stream  = COMM_TX_DMA_STREAM,
    .tx_channel = COMM_TX_DMA_CHANNEL,
};
static uart_instance_t comm_inst;

static bool comm_send(const uint8_t *data, size_t len)
{
    return uart_send(&comm_inst, data, len);
}

/* ── Апаратні об'єкти ──────────────────────────────────────────────────────── */
#define CURRENT_DT_S            0.0002f   /* 5 кГц */
#define STOP_VEL_THRESHOLD_RAD_S 0.05f

typedef enum { APP_RUNNING, APP_STOPPING, APP_STOPPED, APP_ESTOP } App_State_t;

static PWM_Motor_Driver_t           motor;
static HWD_PWM_Handle_t             pwm_fwd;
static Incremental_Encoder_Driver_t encoder;
static GPIO_Brake_Driver_t          brake;
static ACS712_Driver_t              current_driver;
static HWD_ADC_Handle_t             current_adc;
static Cascade_Controller_t         cascade;
static App_State_t                  app_state = APP_STOPPED;

static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

/* ── Спільний стан (запис: task_current, читання: task_control) ─────────────── */
static volatile float s_current_a = 0.0f;

/* Лише для task_control: */
static float s_pos_rad      = 0.0f;
static float s_vel_rad_s    = 0.0f;
static float s_accel_rad_s2 = 0.0f;

/* ── FreeRTOS handles ──────────────────────────────────────────────────────── */
static TaskHandle_t h_current;

/* ============================================================================
 * TIM11 — генератор 5 кГц для task_current
 * APB2 clock = SYSTEM_CORE_CLOCK = 100 МГц → period = 100e6/5000 - 1 = 19999
 * TIM1/TIM2/TIM3/TIM4 зайняті encoder-драйвером — використовуємо TIM11.
 * ============================================================================ */
static void tim11_5khz_init(void)
{
    rcc_periph_clock_enable(RCC_TIM11);
    timer_set_prescaler(TIM11, 0U);
    timer_set_period(TIM11, (SYSTEM_CORE_CLOCK / 5000U) - 1U);
    TIM_EGR(TIM11) = TIM_EGR_UG;
    timer_enable_irq(TIM11, TIM_DIER_UIE);
    nvic_set_priority(NVIC_TIM1_TRG_COM_TIM11_IRQ,
                      configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << 4);
    nvic_enable_irq(NVIC_TIM1_TRG_COM_TIM11_IRQ);
    timer_enable_counter(TIM11);
}

/* TIM11 Update ISR — будить task_current */
void tim1_trg_com_tim11_isr(void)
{
    timer_clear_flag(TIM11, TIM_SR_UIF);
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(h_current, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ============================================================================
 * Задачі
 * ============================================================================ */

/* 5 кГц: оновлення UKF струму */
static void task_current(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Current_Sensor_Update(&current_driver.interface, CURRENT_DT_S);
        Current_Sensor_GetCurrent(&current_driver.interface,
                                  (float *)&s_current_a);
    }
}

/* 1 кГц: енкодер UKF + каскадний PID + comm RX */
static void task_control(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, 1U);

        uint32_t now = Time_GetMicros();

        Position_Sensor_Update(&encoder.interface);
        Position_Sensor_GetPosition(&encoder.interface,     &s_pos_rad);
        Position_Sensor_GetVelocity(&encoder.interface,     &s_vel_rad_s);
        Position_Sensor_GetAcceleration(&encoder.interface, &s_accel_rad_s2);

        Brake_Update(&brake.interface);

        if (app_state == APP_STOPPING &&
            fabsf(s_vel_rad_s) < STOP_VEL_THRESHOLD_RAD_S)
        {
            Motor_Stop(&motor.interface);
            Brake_Engage(&brake.interface);
            Cascade_Reset(&cascade);
            app_state = APP_STOPPED;
        }

        if (app_state == APP_RUNNING || app_state == APP_STOPPING) {
            float power = Cascade_Compute(&cascade, s_pos_rad, s_vel_rad_s,
                                          s_current_a, now);
            Motor_SetPower(&motor.interface, power);
        }

        /* ── Обробка RX (CRC32 — не в ISR) ─────────────────────────────── */
        servo_comm_process_rx();

        if (servo_comm_get_estop()) {
            Motor_EmergencyStop(&motor.interface);
            Brake_Engage(&brake.interface);
            Cascade_Reset(&cascade);
            app_state = APP_ESTOP;
        }

        if (app_state != APP_ESTOP && servo_comm_get_stop()) {
            Cascade_SetMode(&cascade, CASCADE_MODE_VEL);
            cascade.target_vel = 0.0f;
            app_state = APP_STOPPING;
        }

        servo_command_t cmd;
        if (app_state != APP_ESTOP && servo_comm_get_command(&cmd)) {
            if (app_state == APP_STOPPED) {
                Brake_Release(&brake.interface);
                app_state = APP_RUNNING;
            }
            Cascade_SetMode(&cascade, (Cascade_Mode_t)cmd.mode);
            switch ((Cascade_Mode_t)cmd.mode) {
                case CASCADE_MODE_POS: cascade.target_pos     = cmd.target; break;
                case CASCADE_MODE_VEL: cascade.target_vel     = cmd.target; break;
                case CASCADE_MODE_TRQ: cascade.target_current = cmd.target; break;
                default: break;
            }
        }

        cascade_config_t wire_cfg;
        if (servo_comm_get_cascade_config(&wire_cfg)) {
            const Cascade_Config_t new_cfg = {
                .pos = { .kp = wire_cfg.pos_kp, .ki = wire_cfg.pos_ki,
                         .kd = wire_cfg.pos_kd, .out_min = wire_cfg.pos_out_min,
                         .out_max = wire_cfg.pos_out_max, .i_limit = wire_cfg.pos_i_limit },
                .vel = { .kp = wire_cfg.vel_kp, .ki = wire_cfg.vel_ki,
                         .kd = wire_cfg.vel_kd, .out_min = wire_cfg.vel_out_min,
                         .out_max = wire_cfg.vel_out_max, .i_limit = wire_cfg.vel_i_limit },
                .trq = { .kp = wire_cfg.trq_kp, .ki = wire_cfg.trq_ki,
                         .kd = wire_cfg.trq_kd, .out_min = wire_cfg.trq_out_min,
                         .out_max = wire_cfg.trq_out_max, .i_limit = wire_cfg.trq_i_limit },
                .ff_r    = wire_cfg.ff_j,
                .ff_bemf = wire_cfg.ff_b,
                .slew_rate = wire_cfg.slew_rate,
            };
            Cascade_ApplyConfig(&cascade, &new_cfg);
            servo_comm_send_cascade_config(&wire_cfg);
        }
    }
}

/* 100 Гц: телеметрія */
static void task_telem(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, 10U);

        float target = 0.0f;
        switch (cascade.mode) {
            case CASCADE_MODE_POS: target = cascade.target_pos;     break;
            case CASCADE_MODE_VEL: target = cascade.target_vel;     break;
            case CASCADE_MODE_TRQ: target = cascade.target_current; break;
            default: break;
        }

        cascade_telemetry_t telem = {
            .timestamp_ms   = (uint32_t)xTaskGetTickCount(),
            .position_rad   = s_pos_rad,
            .velocity_rad_s = s_vel_rad_s,
            .accel_rad_s2   = s_accel_rad_s2,
            .current_a      = s_current_a,
            .target         = target,
            .vel_sp         = cascade.last_vel_sp,
            .current_sp     = cascade.last_current_sp,
            .ff             = cascade.last_ff,
            .power          = cascade.last_power,
            .pos_p          = cascade.pos_pid.p_term,
            .pos_i          = cascade.pos_pid.i_term,
            .pos_d          = cascade.pos_pid.d_term,
            .vel_p          = cascade.vel_pid.p_term,
            .vel_i          = cascade.vel_pid.i_term,
            .vel_d          = cascade.vel_pid.d_term,
            .vel_integral   = cascade.vel_pid.integral,
            .trq_p          = cascade.trq_pid.p_term,
            .trq_i          = cascade.trq_pid.i_term,
            .trq_d          = cascade.trq_pid.d_term,
            .trq_integral   = cascade.trq_pid.integral,
            .mode           = (uint8_t)cascade.mode,
        };
        servo_comm_send_cascade(&telem);

        HWD_GPIO_TogglePin(&led_pin);
    }
}

/* ============================================================================
 * FreeRTOS хуки
 * ============================================================================ */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task; (void)name;
    Motor_EmergencyStop(&motor.interface);
    Brake_Engage(&brake.interface);
    __asm volatile("bkpt #0");
}

void vApplicationMallocFailedHook(void)
{
    __asm volatile("bkpt #0");
}

/* ============================================================================
 * Main
 * ============================================================================ */
int main(void)
{
    Board_Init();
    frame_crc32_init();

    /* ── Comm ──────────────────────────────────────────────────────────────── */
    servo_comm_init(comm_send);
    uart_init(&comm_inst, &comm_hw, COMM_BAUD, servo_comm_on_rx);

    /* ── ACS712 ────────────────────────────────────────────────────────────── */
    static const HWD_ADC_Config_t adc_cfg = {
        .adc_base  = CURRENT_ADC_PERIPH,  .rcc_adc   = CURRENT_ADC_RCC,
        .rcc_gpio  = CURRENT0_ADC_GPIO_RCC,.gpio_port = CURRENT0_ADC_GPIO_PORT,
        .gpio_pin  = CURRENT0_ADC_GPIO_PIN,.channel   = CURRENT0_ADC_CHANNEL,
        .vref_v    = CURRENT_ADC_VREF_V,
    };
    HWD_ADC_Init(&current_adc, &adc_cfg);
    HWD_ADC_StartScan();

    static const ACS712_Config_t acs_cfg = {
        .variant = ACS712_30A, .adc = &current_adc, .divider_ratio = 0.65f,
    };
    static const Current_Params_t current_params = {
        .overcurrent_threshold_a = 4.0f,
        .process_noise_q         = 0.0001f,
        .measurement_noise_r     = 0.5f,
    };
    ACS712_Create(&current_driver, &acs_cfg, &current_params);
    Time_DelayMs(1000U);
    Current_Sensor_Calibrate(&current_driver.interface);

    /* ── PWM + Motor ───────────────────────────────────────────────────────── */
    HWD_PWM_Config_t fwd_cfg = {
        .frequency  = MOTOR_PWM_FREQ, .resolution = MOTOR_PWM_PERIOD,
        .hw_handle  = (void*)MOTOR0_PWM_TIMER, .hw_channel = MOTOR0_PWM_OC,
    };
    HWD_PWM_Init(&pwm_fwd, &fwd_cfg);

    PWM_Motor_Config_t mot_cfg = {
        .type = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,
        .pwm_fwd = &pwm_fwd, .pwm_bwd = NULL,
        .gpio_dir = (void*)MOTOR0_DIR_GPIO_PORT, .gpio_pin = MOTOR0_DIR_PIN,
    };
    PWM_Motor_Create(&motor, &mot_cfg);

    Motor_Params_t mot_params = {
        .max_power = 99.9f, .min_power = 1.0f, .invert_direction = false,
    };
    Motor_Init(&motor.interface, &mot_params);

    /* ── Encoder ───────────────────────────────────────────────────────────── */
    static const Incremental_Encoder_HW_t enc_hw = {
        .gpio_port_a = ENC0_GPIO_PORT_A, .gpio_pin_a = ENC0_GPIO_PIN_A,
        .gpio_af_a   = ENC0_GPIO_AF,
        .gpio_port_b = ENC0_GPIO_PORT_B, .gpio_pin_b = ENC0_GPIO_PIN_B,
        .timer_base  = ENC0_TIMER_BASE,  .timer_rcc  = ENC0_TIMER_RCC,
        .ic_channel  = 0U,
    };
    Incremental_Encoder_Create(&encoder, ENC0_CPR, &enc_hw);
    Position_Sensor_Init(&encoder.interface);

    /* ── Brake ─────────────────────────────────────────────────────────────── */
    GPIO_Brake_Config_t brk_cfg = {
        .gpio_port = (void*)BRAKE0_GPIO_PORT, .gpio_pin = BRAKE0_PIN,
        .active_high = false, .engage_time_ms = 50, .release_time_ms = 30,
    };
    GPIO_Brake_Create(&brake, &brk_cfg);

    /* ── Cascade PID ───────────────────────────────────────────────────────── */
    static const Cascade_Config_t casc_cfg = {
        .pos = { .kp=6.5f, .ki=0.0f, .kd=0.1f,
                 .out_min=-0.2f, .out_max=0.2f, .i_limit=0.1f },
        .vel = { .kp=8.0f, .ki=73.0f, .kd=0.0f,
                 .out_min=-4.0f, .out_max=4.0f, .i_limit=3.0f },
        .trq = { .kp=0.0f, .ki=100.0f, .kd=0.0f,
                 .out_min=-100.0f, .out_max=100.0f, .i_limit=40.0f },
        .ff_r=11.15f, .ff_bemf=120.0f, .slew_rate=5000.0f,
    };
    Cascade_Init(&cascade, &casc_cfg, CASCADE_MODE_POS);

    const cascade_config_t seed = {
        .pos_kp=casc_cfg.pos.kp, .pos_ki=casc_cfg.pos.ki, .pos_kd=casc_cfg.pos.kd,
        .pos_out_min=casc_cfg.pos.out_min, .pos_out_max=casc_cfg.pos.out_max,
        .pos_i_limit=casc_cfg.pos.i_limit,
        .vel_kp=casc_cfg.vel.kp, .vel_ki=casc_cfg.vel.ki, .vel_kd=casc_cfg.vel.kd,
        .vel_out_min=casc_cfg.vel.out_min, .vel_out_max=casc_cfg.vel.out_max,
        .vel_i_limit=casc_cfg.vel.i_limit,
        .trq_kp=casc_cfg.trq.kp, .trq_ki=casc_cfg.trq.ki, .trq_kd=casc_cfg.trq.kd,
        .trq_out_min=casc_cfg.trq.out_min, .trq_out_max=casc_cfg.trq.out_max,
        .trq_i_limit=casc_cfg.trq.i_limit,
        .ff_j=casc_cfg.ff_r, .ff_b=casc_cfg.ff_bemf, .slew_rate=casc_cfg.slew_rate,
    };
    servo_comm_seed_cascade_config(&seed);

    /* ── TIM11 для task_current (5 кГц) ───────────────────────────────────── */
    tim11_5khz_init();

    /* ── Задачі ────────────────────────────────────────────────────────────── */
    xTaskCreate(task_current, "cur",  256, NULL, 4, &h_current);
    xTaskCreate(task_control, "ctrl", 512, NULL, 3, NULL);
    xTaskCreate(task_telem,   "tlm",  256, NULL, 2, NULL);

    vTaskStartScheduler();
    for (;;) {}
}
