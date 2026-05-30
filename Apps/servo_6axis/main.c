/* ============================================================================
 * servo_6axis — 6-осьовий серво-контролер з FreeRTOS
 *
 * Плата:  STM32F411_ServoMaster
 * Задачі:
 *   task_current (5 кГц) — TIM11 IRQ → xTaskNotifyFromISR
 *                           Current_Sensor_Update × 6
 *   task_control (1 кГц) — vTaskDelayUntil
 *                           Cascade_Compute × 6 + Brake_Update × 6 + comm RX
 *   task_telem  (100 Гц) — vTaskDelayUntil → cascade_telemetry осі 0
 *
 * Позиція осей: axes[i].pos_rad = 0.0f (заглушка до реалізації EncoderHub SPI).
 * Команди від хоста транслюються на всі 6 осей одночасно (broadcast).
 * Після реалізації EHUB протоколу:
 *   у task_control → ehub_read_all(&telem)
 *   axes[i].pos_rad    = telem.ch[i].angle_rad
 *   axes[i].vel_rad_s  = ehub_omega_decode(telem.ch[i].omega_crad_s)
 * ============================================================================ */

#include "FreeRTOS.h"
#include "task.h"

#include "board_config.h"
#include "ctrl/cascade.h"
#include "drv/motor/pwm.h"
#include "drv/brake/gpio_brake.h"
#include "drv/current/acs712.h"
#include "hwd/hwd_pwm.h"
#include "hwd/hwd_adc.h"
#include "hwd/hwd_gpio.h"
#include "hwd/hwd_spi.h"
#include "hwd/hwd_timer.h"
#include "comm/servo_comm.h"
#include "hwd_uart_async.h"

#include <math.h>
#include <string.h>

/* ── Константи ──────────────────────────────────────────────────────────────── */
#define AXIS_COUNT              6U
#define CURRENT_DT_S            0.0002f   /* 5 кГц */
#define STOP_VEL_THRESHOLD_RAD_S 0.05f

/* ── UART async ─────────────────────────────────────────────────────────────── */
static const uart_hw_config_t comm_hw = {
    .usart      = COMM_USART,
    .tx_port    = COMM_GPIO_PORT, .tx_pin    = COMM_TX_PIN,
    .rx_port    = COMM_GPIO_PORT, .rx_pin    = COMM_RX_PIN,
    .rx_dma     = COMM_RX_DMA,
    .rx_stream  = COMM_RX_DMA_STREAM, .rx_channel = COMM_RX_DMA_CHANNEL,
    .tx_dma     = COMM_TX_DMA,
    .tx_stream  = COMM_TX_DMA_STREAM, .tx_channel = COMM_TX_DMA_CHANNEL,
};
static uart_instance_t comm_inst;

static bool comm_send(const uint8_t *data, size_t len)
{
    return uart_send(&comm_inst, data, len);
}

/* ── Типи ───────────────────────────────────────────────────────────────────── */
typedef enum { APP_RUNNING, APP_STOPPING, APP_STOPPED, APP_ESTOP } App_State_t;

typedef struct {
    PWM_Motor_Driver_t   motor;
    HWD_PWM_Handle_t     pwm;
    GPIO_Brake_Driver_t  brake;
    ACS712_Driver_t      current;
    HWD_ADC_Handle_t     adc;
    Cascade_Controller_t cascade;
    volatile float       current_a;   /* запис: task_current; читання: task_control */
    float                pos_rad;     /* EncoderHub → заглушка 0.0f */
    float                vel_rad_s;   /* EncoderHub → заглушка 0.0f */
    App_State_t          state;
} Axis_t;

static Axis_t axes[AXIS_COUNT];

/* ── SPI handle для EncoderHub (ініціалізується, протокол — пізніше) ─────────── */
static HWD_SPI_Handle_t ehub_spi;

/* ── LED ────────────────────────────────────────────────────────────────────── */
static const HWD_GPIO_Pin_t led_pin = {
    .port = (void*)LED_GPIO_PORT,
    .pin  = LED_PIN,
    .mode = HWD_GPIO_MODE_OUTPUT,
    .pull = HWD_GPIO_NOPULL,
};

/* ============================================================================
 * Таблиці апаратних конфігурацій по осях (з board_config.h)
 * ============================================================================ */
static const struct {
    void    *pwm_timer;
    uint32_t pwm_oc;
    void    *dir_port;
    uint32_t dir_pin;
} k_motor_hw[AXIS_COUNT] = {
    { (void*)MOTOR0_PWM_TIMER, MOTOR0_PWM_OC, (void*)MOTOR0_DIR_GPIO_PORT, MOTOR0_DIR_PIN },
    { (void*)MOTOR1_PWM_TIMER, MOTOR1_PWM_OC, (void*)MOTOR1_DIR_GPIO_PORT, MOTOR1_DIR_PIN },
    { (void*)MOTOR2_PWM_TIMER, MOTOR2_PWM_OC, (void*)MOTOR2_DIR_GPIO_PORT, MOTOR2_DIR_PIN },
    { (void*)MOTOR3_PWM_TIMER, MOTOR3_PWM_OC, (void*)MOTOR3_DIR_GPIO_PORT, MOTOR3_DIR_PIN },
    { (void*)MOTOR4_PWM_TIMER, MOTOR4_PWM_OC, (void*)MOTOR4_DIR_GPIO_PORT, MOTOR4_DIR_PIN },
    { (void*)MOTOR5_PWM_TIMER, MOTOR5_PWM_OC, (void*)MOTOR5_DIR_GPIO_PORT, MOTOR5_DIR_PIN },
};

static const struct {
    void    *port;
    uint32_t pin;
} k_brake_hw[AXIS_COUNT] = {
    { (void*)BRAKE0_GPIO_PORT, BRAKE0_PIN },
    { (void*)BRAKE1_GPIO_PORT, BRAKE1_PIN },
    { (void*)BRAKE2_GPIO_PORT, BRAKE2_PIN },
    { (void*)BRAKE3_GPIO_PORT, BRAKE3_PIN },
    { (void*)BRAKE4_GPIO_PORT, BRAKE4_PIN },
    { (void*)BRAKE5_GPIO_PORT, BRAKE5_PIN },
};

static const HWD_ADC_Config_t k_adc_cfg[AXIS_COUNT] = {
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT0_ADC_GPIO_RCC,
      (uint32_t)CURRENT0_ADC_GPIO_PORT, CURRENT0_ADC_GPIO_PIN, CURRENT0_ADC_CHANNEL, CURRENT_ADC_VREF_V },
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT1_ADC_GPIO_RCC,
      (uint32_t)CURRENT1_ADC_GPIO_PORT, CURRENT1_ADC_GPIO_PIN, CURRENT1_ADC_CHANNEL, CURRENT_ADC_VREF_V },
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT2_ADC_GPIO_RCC,
      (uint32_t)CURRENT2_ADC_GPIO_PORT, CURRENT2_ADC_GPIO_PIN, CURRENT2_ADC_CHANNEL, CURRENT_ADC_VREF_V },
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT3_ADC_GPIO_RCC,
      (uint32_t)CURRENT3_ADC_GPIO_PORT, CURRENT3_ADC_GPIO_PIN, CURRENT3_ADC_CHANNEL, CURRENT_ADC_VREF_V },
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT4_ADC_GPIO_RCC,
      (uint32_t)CURRENT4_ADC_GPIO_PORT, CURRENT4_ADC_GPIO_PIN, CURRENT4_ADC_CHANNEL, CURRENT_ADC_VREF_V },
    { CURRENT_ADC_PERIPH, CURRENT_ADC_RCC, CURRENT5_ADC_GPIO_RCC,
      (uint32_t)CURRENT5_ADC_GPIO_PORT, CURRENT5_ADC_GPIO_PIN, CURRENT5_ADC_CHANNEL, CURRENT_ADC_VREF_V },
};

/* ── Спільні параметри для всіх осей (початкові — тюнінг per-axis via комм) ── */
static const Motor_Params_t k_motor_params = {
    .max_power        = 99.9f,
    .min_power        = 1.0f,
    .invert_direction = false,
};

static const ACS712_Config_t k_acs_cfg = {
    .variant       = ACS712_30A,
    .divider_ratio = 0.65f,
    /* .adc заповнюється у init_axis() */
};

static const Current_Params_t k_current_params = {
    .overcurrent_threshold_a = 4.0f,
    .process_noise_q         = 0.0001f,
    .measurement_noise_r     = 0.5f,
};

static const Cascade_Config_t k_casc_cfg = {
    .pos = { .kp=6.5f, .ki=0.0f,  .kd=0.1f,
             .out_min=-0.2f,  .out_max=0.2f,   .i_limit=0.1f  },
    .vel = { .kp=8.0f, .ki=73.0f, .kd=0.0f,
             .out_min=-4.0f,  .out_max=4.0f,   .i_limit=3.0f  },
    .trq = { .kp=0.0f, .ki=100.0f,.kd=0.0f,
             .out_min=-100.0f,.out_max=100.0f,  .i_limit=40.0f },
    .ff_r      = 11.15f,
    .ff_bemf   = 120.0f,
    .slew_rate = 5000.0f,
};

/* ── FreeRTOS handle для task_current ──────────────────────────────────────── */
static TaskHandle_t h_current;

/* ============================================================================
 * TIM11 — генератор 5 кГц для task_current
 * APB2 = 100 МГц → period = 100e6/5000 - 1 = 19999
 * TIM1/TIM3 зайняті PWM — TIM11 (shared IRQ з TIM1 trig/com, але без конфлікту)
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

void tim1_trg_com_tim11_isr(void)
{
    timer_clear_flag(TIM11, TIM_SR_UIF);
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(h_current, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ============================================================================
 * Аварійна зупинка всіх осей
 * ============================================================================ */
static void estop_all(void)
{
    for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
        Motor_EmergencyStop(&axes[i].motor.interface);
        Brake_Engage(&axes[i].brake.interface);
        Cascade_Reset(&axes[i].cascade);
        axes[i].state = APP_ESTOP;
    }
}

/* ============================================================================
 * Задачі
 * ============================================================================ */

/* 5 кГц: UKF струму для всіх 6 осей (~6 × 5–10 мкс = 60 мкс / 200 мкс) */
static void task_current(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
            Current_Sensor_Update(&axes[i].current.interface, CURRENT_DT_S);
            Current_Sensor_GetCurrent(&axes[i].current.interface,
                                      (float *)&axes[i].current_a);
        }
    }
}

/* 1 кГц: Cascade_Compute × 6 + Brake_Update × 6 + comm RX */
static void task_control(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, 1U);

        uint32_t now = HWD_Timer_GetMicros();

        /* ── Апаратний E-STOP (PC15) ──────────────────────────────────────── */
        if (Board_IsEstopActive()) {
            estop_all();
            Board_ClearEstop();
        }

        /* ── Оновлення кожної осі ─────────────────────────────────────────── */
        for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
            Brake_Update(&axes[i].brake.interface);

            if (axes[i].state == APP_STOPPING &&
                fabsf(axes[i].vel_rad_s) < STOP_VEL_THRESHOLD_RAD_S)
            {
                Motor_Stop(&axes[i].motor.interface);
                Brake_Engage(&axes[i].brake.interface);
                Cascade_Reset(&axes[i].cascade);
                axes[i].state = APP_STOPPED;
            }

            if (axes[i].state == APP_RUNNING || axes[i].state == APP_STOPPING) {
                float power = Cascade_Compute(&axes[i].cascade,
                                              axes[i].pos_rad,
                                              axes[i].vel_rad_s,
                                              axes[i].current_a,
                                              now);
                Motor_SetPower(&axes[i].motor.interface, power);
            }
        }

        /* ── Обробка RX (CRC32 — не в ISR) ──────────────────────────────── */
        servo_comm_process_rx();

        /* Програмний E-STOP від хоста */
        if (servo_comm_get_estop()) {
            estop_all();
        }

        /* Зупинка */
        if (servo_comm_get_stop()) {
            for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
                if (axes[i].state == APP_RUNNING) {
                    Cascade_SetMode(&axes[i].cascade, CASCADE_MODE_VEL);
                    axes[i].cascade.target_vel = 0.0f;
                    axes[i].state = APP_STOPPING;
                }
            }
        }

        /* Команда руху — broadcast на всі осі */
        servo_command_t cmd;
        if (servo_comm_get_command(&cmd)) {
            for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
                if (axes[i].state == APP_ESTOP) continue;
                if (axes[i].state == APP_STOPPED) {
                    Brake_Release(&axes[i].brake.interface);
                    axes[i].state = APP_RUNNING;
                }
                Cascade_SetMode(&axes[i].cascade, (Cascade_Mode_t)cmd.mode);
                switch ((Cascade_Mode_t)cmd.mode) {
                    case CASCADE_MODE_POS: axes[i].cascade.target_pos     = cmd.target; break;
                    case CASCADE_MODE_VEL: axes[i].cascade.target_vel     = cmd.target; break;
                    case CASCADE_MODE_TRQ: axes[i].cascade.target_current = cmd.target; break;
                    default: break;
                }
            }
        }

        /* Online PID тюнінг — застосовується до всіх осей */
        cascade_config_t wire_cfg;
        if (servo_comm_get_cascade_config(&wire_cfg)) {
            const Cascade_Config_t new_cfg = {
                .pos = { .kp=wire_cfg.pos_kp, .ki=wire_cfg.pos_ki, .kd=wire_cfg.pos_kd,
                         .out_min=wire_cfg.pos_out_min, .out_max=wire_cfg.pos_out_max,
                         .i_limit=wire_cfg.pos_i_limit },
                .vel = { .kp=wire_cfg.vel_kp, .ki=wire_cfg.vel_ki, .kd=wire_cfg.vel_kd,
                         .out_min=wire_cfg.vel_out_min, .out_max=wire_cfg.vel_out_max,
                         .i_limit=wire_cfg.vel_i_limit },
                .trq = { .kp=wire_cfg.trq_kp, .ki=wire_cfg.trq_ki, .kd=wire_cfg.trq_kd,
                         .out_min=wire_cfg.trq_out_min, .out_max=wire_cfg.trq_out_max,
                         .i_limit=wire_cfg.trq_i_limit },
                .ff_r=wire_cfg.ff_j, .ff_bemf=wire_cfg.ff_b, .slew_rate=wire_cfg.slew_rate,
            };
            for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
                Cascade_ApplyConfig(&axes[i].cascade, &new_cfg);
            }
            servo_comm_send_cascade_config(&wire_cfg);
        }
    }
}

/* 100 Гц: телеметрія осі 0 */
static void task_telem(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, 10U);

        const Axis_t *ax = &axes[0];
        float target = 0.0f;
        switch (ax->cascade.mode) {
            case CASCADE_MODE_POS: target = ax->cascade.target_pos;     break;
            case CASCADE_MODE_VEL: target = ax->cascade.target_vel;     break;
            case CASCADE_MODE_TRQ: target = ax->cascade.target_current; break;
            default: break;
        }

        cascade_telemetry_t telem = {
            .timestamp_ms   = (uint32_t)xTaskGetTickCount(),
            .position_rad   = ax->pos_rad,
            .velocity_rad_s = ax->vel_rad_s,
            .accel_rad_s2   = 0.0f,
            .current_a      = ax->current_a,
            .target         = target,
            .vel_sp         = ax->cascade.last_vel_sp,
            .current_sp     = ax->cascade.last_current_sp,
            .ff             = ax->cascade.last_ff,
            .power          = ax->cascade.last_power,
            .pos_p          = ax->cascade.pos_pid.p_term,
            .pos_i          = ax->cascade.pos_pid.i_term,
            .pos_d          = ax->cascade.pos_pid.d_term,
            .vel_p          = ax->cascade.vel_pid.p_term,
            .vel_i          = ax->cascade.vel_pid.i_term,
            .vel_d          = ax->cascade.vel_pid.d_term,
            .vel_integral   = ax->cascade.vel_pid.integral,
            .trq_p          = ax->cascade.trq_pid.p_term,
            .trq_i          = ax->cascade.trq_pid.i_term,
            .trq_d          = ax->cascade.trq_pid.d_term,
            .trq_integral   = ax->cascade.trq_pid.integral,
            .mode           = (uint8_t)ax->cascade.mode,
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
    estop_all();
    __asm volatile("bkpt #0");
}

void vApplicationMallocFailedHook(void)
{
    __asm volatile("bkpt #0");
}

/* ============================================================================
 * Ініціалізація однієї осі
 * ============================================================================ */
static void init_axis(uint8_t i)
{
    /* PWM */
    const HWD_PWM_Config_t pwm_cfg = {
        .frequency  = MOTOR_PWM_FREQ,
        .resolution = MOTOR_PWM_PERIOD,
        .hw_handle  = k_motor_hw[i].pwm_timer,
        .hw_channel = k_motor_hw[i].pwm_oc,
    };
    HWD_PWM_Init(&axes[i].pwm, &pwm_cfg);

    /* Двигун */
    const PWM_Motor_Config_t mot_cfg = {
        .type     = PWM_MOTOR_TYPE_SINGLE_PWM_DIR,
        .pwm_fwd  = &axes[i].pwm,
        .pwm_bwd  = NULL,
        .gpio_dir = k_motor_hw[i].dir_port,
        .gpio_pin = k_motor_hw[i].dir_pin,
    };
    PWM_Motor_Create(&axes[i].motor, &mot_cfg);
    Motor_Init(&axes[i].motor.interface, &k_motor_params);

    /* Гальмо */
    const GPIO_Brake_Config_t brk_cfg = {
        .gpio_port       = k_brake_hw[i].port,
        .gpio_pin        = k_brake_hw[i].pin,
        .active_high     = false,
        .engage_time_ms  = 50,
        .release_time_ms = 30,
    };
    GPIO_Brake_Create(&axes[i].brake, &brk_cfg);

    /* Датчик струму */
    HWD_ADC_Init(&axes[i].adc, &k_adc_cfg[i]);

    ACS712_Config_t acs = k_acs_cfg;
    acs.adc = &axes[i].adc;
    ACS712_Create(&axes[i].current, &acs, &k_current_params);

    /* Каскадний PID */
    Cascade_Init(&axes[i].cascade, &k_casc_cfg, CASCADE_MODE_POS);
    axes[i].cascade.target_pos = 0.0f;
    axes[i].state = APP_STOPPED;
}

/* ============================================================================
 * Main
 * ============================================================================ */
int main(void)
{
    Board_Init();
    frame_crc32_init();

    /* ── Comm ─────────────────────────────────────────────────────────────── */
    servo_comm_init(comm_send);
    uart_init(&comm_inst, &comm_hw, COMM_BAUD, servo_comm_on_rx);

    /* ── Ініціалізація всіх 6 осей ────────────────────────────────────────── */
    for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
        init_axis(i);
    }

    /* ── ADC scan (один раз після всіх HWD_ADC_Init) ─────────────────────── */
    HWD_ADC_StartScan();

    /* ── Калібрування нульового зміщення ACS712 (при нульовому струмі) ──── */
    HWD_Timer_DelayMs(500U);
    for (uint8_t i = 0U; i < AXIS_COUNT; i++) {
        Current_Sensor_Calibrate(&axes[i].current.interface);
    }

    /* ── EncoderHub SPI (підготовка handle — протокол реалізується окремо) ── */
    const HWD_SPI_Config_t spi_cfg = {
        .spi_handle = (void*)EHUB_SPI,
        .cs_port    = (void*)EHUB_SPI_NSS_PORT,
        .cs_pin     = EHUB_SPI_NSS_PIN,
        .timeout_ms = 10U,
    };
    HWD_SPI_Init(&ehub_spi, &spi_cfg);

    /* ── Seed для online PID тюнінгу (за конфігом осі 0) ─────────────────── */
    const cascade_config_t seed = {
        .pos_kp=k_casc_cfg.pos.kp, .pos_ki=k_casc_cfg.pos.ki, .pos_kd=k_casc_cfg.pos.kd,
        .pos_out_min=k_casc_cfg.pos.out_min, .pos_out_max=k_casc_cfg.pos.out_max,
        .pos_i_limit=k_casc_cfg.pos.i_limit,
        .vel_kp=k_casc_cfg.vel.kp, .vel_ki=k_casc_cfg.vel.ki, .vel_kd=k_casc_cfg.vel.kd,
        .vel_out_min=k_casc_cfg.vel.out_min, .vel_out_max=k_casc_cfg.vel.out_max,
        .vel_i_limit=k_casc_cfg.vel.i_limit,
        .trq_kp=k_casc_cfg.trq.kp, .trq_ki=k_casc_cfg.trq.ki, .trq_kd=k_casc_cfg.trq.kd,
        .trq_out_min=k_casc_cfg.trq.out_min, .trq_out_max=k_casc_cfg.trq.out_max,
        .trq_i_limit=k_casc_cfg.trq.i_limit,
        .ff_j=k_casc_cfg.ff_r, .ff_b=k_casc_cfg.ff_bemf, .slew_rate=k_casc_cfg.slew_rate,
    };
    servo_comm_seed_cascade_config(&seed);

    /* ── TIM11 5 кГц ──────────────────────────────────────────────────────── */
    tim11_5khz_init();

    /* ── FreeRTOS задачі ──────────────────────────────────────────────────── */
    xTaskCreate(task_current, "cur",  384, NULL, 4, &h_current);
    xTaskCreate(task_control, "ctrl", 768, NULL, 3, NULL);
    xTaskCreate(task_telem,   "tlm",  256, NULL, 2, NULL);

    vTaskStartScheduler();
    for (;;) {}
}
