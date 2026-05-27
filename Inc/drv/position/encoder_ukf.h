/**
 * @file encoder_ukf.h
 * @brief UKF фільтр для інкрементального енкодера
 *
 * Стан: [θ (рад), ω (рад/с), α (рад/с²)]
 * Вимірювання: [θ, ω] — θ з лічильника EXTI X4, ω з IC таймера
 *
 * ω-вимірювання умовне: коли IC таймер не дав нового замір з минулого
 * кроку, R[1,1] підвищується до ENCODER_UKF_R_OMEGA_STALE — UKF ігнорує ω.
 * Це коректно обробляє змінний інтервал IC (23 мкс … 65 мс).
 *
 * Модель переходу (кінематика):
 *   θ(k+1) = θ(k) + ω·dt + 0.5·α·dt²
 *   ω(k+1) = ω(k) + α·dt
 *   α(k+1) = α(k)
 */

#ifndef SERVOCORE_DRV_ENCODER_UKF_H
#define SERVOCORE_DRV_ENCODER_UKF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../core.h"
#include "ukf_mcu.h"

/* Розміри стану/вимірювання -------------------------------------------------*/
#define ENCODER_UKF_N_STATES  3U   /**< θ, ω, α */
#define ENCODER_UKF_N_MEAS    2U   /**< θ (завжди), ω (умовно — з IC таймера) */

/* Значення за замовчуванням — підбирати на реальному стенді -----------------*/
#define ENCODER_UKF_Q_THETA_DEFAULT      1e-6f   /**< шум моделі: позиція (рад²) */
#define ENCODER_UKF_Q_OMEGA_DEFAULT      1e-4f   /**< шум моделі: швидкість (рад²/с²) */
#define ENCODER_UKF_Q_ALPHA_DEFAULT      1e-2f   /**< шум моделі: прискорення (рад²/с⁴) */
#define ENCODER_UKF_R_THETA_DEFAULT      1e-4f   /**< шум вимірювання θ (рад²) */
#define ENCODER_UKF_R_OMEGA_DEFAULT      0.1f    /**< шум вимірювання ω з IC ((рад/с)²) */
#define ENCODER_UKF_R_OMEGA_STALE        1e6f    /**< R[1,1] коли немає свіжого IC виміру */
#define ENCODER_UKF_P0_THETA_DEFAULT     0.1f    /**< початкова невизначеність: θ */
#define ENCODER_UKF_P0_OMEGA_DEFAULT     1.0f    /**< початкова невизначеність: ω */
#define ENCODER_UKF_P0_ALPHA_DEFAULT     10.0f   /**< початкова невизначеність: α */

/* Типи ----------------------------------------------------------------------*/

/**
 * @brief Параметри шуму та початкової коваріації
 */
typedef struct {
    float q_theta;   /**< шум процесу: позиція */
    float q_omega;   /**< шум процесу: швидкість */
    float q_alpha;   /**< шум процесу: прискорення */
    float r_theta;   /**< шум вимірювання θ (рад²) */
    float r_omega;   /**< шум вимірювання ω з IC ((рад/с)²) */
    float p0_theta;  /**< початкова дисперсія: θ */
    float p0_omega;  /**< початкова дисперсія: ω */
    float p0_alpha;  /**< початкова дисперсія: α */
} Encoder_UKF_Config_t;

/**
 * @brief Екземпляр фільтра одного енкодера
 *
 * buffer — зовнішній буфер пам'яті для ukf_t (ukf_mcu EncoderHub API).
 * Розмір вираховується макросом UKF_BUFFER_FLOATS(n_states, n_meas).
 */
typedef struct {
    ukf_t  ukf;
    float  buffer[UKF_BUFFER_FLOATS(ENCODER_UKF_N_STATES, ENCODER_UKF_N_MEAS)];
    float  r_omega;      /**< R[1,1] при свіжому IC вимірі (кешується з конфігурації) */
    float  omega_ic;     /**< останній ω з IC (рад/с); 0 до першого FeedOmega */
    bool   omega_fresh;  /**< true якщо FeedOmega викликався після останнього Update */
} Encoder_UKF_t;

/* API -----------------------------------------------------------------------*/

/**
 * @brief Ініціалізація фільтра
 *
 * @param filter     Вказівник на екземпляр
 * @param config     Параметри шуму; NULL — використати DEFAULT значення
 * @param theta_init Початковий кут (рад), зазвичай 0
 */
Servo_Status_t Encoder_UKF_Init(Encoder_UKF_t *filter,
                                 const Encoder_UKF_Config_t *config,
                                 float theta_init);

/**
 * @brief Крок predict + update
 *
 * Викликати на кожному кроці оновлення (10 kHz).
 *
 * @param filter     Вказівник на екземпляр
 * @param theta_meas Абсолютний кут з енкодера (необмежений, рад)
 * @param dt         Крок часу (с)
 */
Servo_Status_t Encoder_UKF_Update(Encoder_UKF_t *filter, float theta_meas, float dt);

/**
 * @brief Зчитування відфільтрованого стану
 *
 * @param filter Вказівник на екземпляр
 * @param theta  Вихід: позиція (рад), може бути NULL
 * @param omega  Вихід: швидкість (рад/с), може бути NULL
 * @param alpha  Вихід: прискорення (рад/с²), може бути NULL
 */
void Encoder_UKF_GetState(const Encoder_UKF_t *filter,
                           float *theta, float *omega, float *alpha);

/**
 * @brief Передати нове ω-вимірювання з IC таймера
 *
 * Викликати після Incremental_Encoder_ConsumeIC() повернула true.
 * Встановлює omega_fresh = true — наступний Encoder_UKF_Update
 * використає це значення з повним вагою R[1,1] = r_omega.
 *
 * @param filter       Вказівник на екземпляр
 * @param omega_rad_s  ω з IC таймера (рад/с, зі знаком)
 */
void Encoder_UKF_FeedOmega(Encoder_UKF_t *filter, float omega_rad_s);

/**
 * @brief Скидання стану при відомому куті (напр. після калібрування)
 *
 * @param filter     Вказівник на екземпляр
 * @param theta_init Новий початковий кут (рад)
 */
void Encoder_UKF_Reset(Encoder_UKF_t *filter, float theta_init);

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_ENCODER_UKF_H */
