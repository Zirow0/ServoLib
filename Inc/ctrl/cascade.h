/**
 * @file cascade.h
 * @brief Каскадний PID регулятор з feedforward
 *
 * Три контури керування:
 *   pos-контур:  похибка положення (рад)   → setpoint кутової швидкості (рад/с)
 *   vel-контур:  похибка швидкості (рад/с) → setpoint струму (А)
 *   trq-контур:  похибка струму (А)        → команда двигуну (%)
 *
 * Режими (Cascade_Mode_t):
 *   CASCADE_MODE_POS — усі три контури активні
 *   CASCADE_MODE_VEL — pos-контур вимкнено, vel+trq активні
 *   CASCADE_MODE_TRQ — лише trq-контур (pos і vel вимкнені)
 *
 * Feedforward (фізична модель):
 *   ff_amps = ff_j * alpha + ff_b * omega
 *   де ff_j [А/(рад/с²)] — інерційна компенсація, ff_b [А·с/рад] — компенсація тертя.
 *   FF додається до current_sp і обмежується межами vel-контуру (vel.out_min/max).
 *   У режимі TRQ feedforward не застосовується.
 *
 * Slew rate limiter:
 *   Обмежує швидкість зміни команди двигуну (% /с).
 *   Контролер "бачить" реальну обмежену команду → anti-windup коректний.
 */

#ifndef SERVOCORE_CTRL_CASCADE_H
#define SERVOCORE_CTRL_CASCADE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../core.h"
#include "pid.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Режим роботи каскадного контролера
 */
typedef enum {
    CASCADE_MODE_POS = 0,  /**< Керування положенням (pos → vel → trq) */
    CASCADE_MODE_VEL = 1,  /**< Керування швидкістю (vel → trq) */
    CASCADE_MODE_TRQ = 2,  /**< Керування струмом (лише trq) */
} Cascade_Mode_t;

/**
 * @brief Параметри одного PID контуру каскаду
 */
typedef struct {
    float kp;       /**< Пропорційний коефіцієнт */
    float ki;       /**< Інтегральний коефіцієнт */
    float kd;       /**< Диференціальний коефіцієнт */
    float out_min;  /**< Мінімальне значення виходу */
    float out_max;  /**< Максимальне значення виходу */
    float i_limit;  /**< Макс. внесок інтегратора (в одиницях виходу).
                         0 = обмеження відносно out_min/out_max */
} Cascade_PID_Params_t;

/**
 * @brief Конфігурація каскадного контролера
 *
 * Одиниці виходів:
 *   pos → рад/с (setpoint швидкості)
 *   vel → А     (setpoint струму, обмежує робочий струм)
 *   trq → %     (команда двигуну, -100..100)
 */
typedef struct {
    Cascade_PID_Params_t pos;  /**< Контур положення, вихід рад/с */
    Cascade_PID_Params_t vel;  /**< Контур швидкості, вихід А */
    Cascade_PID_Params_t trq;  /**< Контур струму, вихід % */

    float ff_j;       /**< FF інерції [А/(рад/с²)]: струм на одиницю прискорення */
    float ff_b;       /**< FF тертя [А·с/рад]: струм на одиницю швидкості */
    float slew_rate;  /**< Макс. швидкість зміни команди [%/с], 0 = вимкнено */
} Cascade_Config_t;

/**
 * @brief Стан каскадного контролера
 */
typedef struct {
    Cascade_Config_t config;     /**< Конфігурація */
    Cascade_Mode_t   mode;       /**< Поточний режим */

    PID_Controller_t pos_pid;    /**< PID контуру положення */
    PID_Controller_t vel_pid;    /**< PID контуру швидкості */
    PID_Controller_t trq_pid;    /**< PID контуру струму */

    /* Setpoint'и */
    float target_pos;      /**< Цільове положення (рад) */
    float target_vel;      /**< Цільова кутова швидкість (рад/с) */
    float target_current;  /**< Цільовий струм (А) */

    /* Пам'ять slew rate */
    float    prev_power;    /**< Команда попереднього кроку (%) */
    uint32_t prev_time_us;  /**< Час попереднього кроку (мкс) */

    /* Телеметрія */
    float last_vel_sp;     /**< Останній setpoint швидкості (рад/с) */
    float last_current_sp; /**< Останній setpoint струму (А) */
    float last_ff;         /**< Останній внесок feedforward (А) */
    float last_power;      /**< Остання команда двигуну (%) */
} Cascade_Controller_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Ініціалізація каскадного контролера
 *
 * @param casc   Вказівник на контролер
 * @param config Конфігурація
 * @param mode   Початковий режим
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Cascade_Init(Cascade_Controller_t* casc,
                             const Cascade_Config_t* config,
                             Cascade_Mode_t mode);

/**
 * @brief Обчислення команди двигуну за один крок
 *
 * @param casc      Вказівник на контролер
 * @param theta     Поточне положення (рад)
 * @param omega        Поточна кутова швидкість (рад/с)
 * @param alpha_rad_s2 Поточне кутове прискорення (рад/с²) — для FF компенсації інерції
 * @param current_a    Поточний струм двигуна (А)
 * @param time_us      Поточний час (мкс)
 * @return float       Команда двигуну (%)
 */
float Cascade_Compute(Cascade_Controller_t* casc,
                      float theta,
                      float omega,
                      float alpha_rad_s2,
                      float current_a,
                      uint32_t time_us);

/**
 * @brief Скидання всіх контурів і slew rate
 */
Servo_Status_t Cascade_Reset(Cascade_Controller_t* casc);

/**
 * @brief Зміна режиму з автоматичним скиданням
 */
Servo_Status_t Cascade_SetMode(Cascade_Controller_t* casc, Cascade_Mode_t mode);

/**
 * @brief Застосувати нову конфігурацію без скидання інтеграторів
 *
 * Оновлює коефіцієнти та ліміти всіх трьох PID контурів + FF + slew rate.
 * Інтегральний стан та попередні значення зберігаються — безпечно
 * викликати під час роботи для online тюнінгу.
 *
 * @param casc   Вказівник на контролер
 * @param config Нова конфігурація
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t Cascade_ApplyConfig(Cascade_Controller_t* casc,
                                    const Cascade_Config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_CTRL_CASCADE_H */
