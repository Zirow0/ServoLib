/**
 * @file encoder_ukf.c
 * @brief UKF фільтр для інкрементального енкодера — реалізація
 */

#include "../../../Inc/drv/position/encoder_ukf.h"

/* Модель переходу стану -----------------------------------------------------*/

static void state_func(const float *x, float dt, float *xp, void *ud)
{
    (void)ud;
    float dt2 = dt * dt;
    xp[0] = x[0] + x[1] * dt + 0.5f * x[2] * dt2;  /* θ */
    xp[1] = x[1] + x[2] * dt;                         /* ω */
    xp[2] = x[2];                                      /* α */
}

/* Модель вимірювання --------------------------------------------------------*/

static void meas_func(const float *x, float *z, void *ud)
{
    (void)ud;
    z[0] = x[0];  /* θ */
    z[1] = x[1];  /* ω — кондиціонується через R[1,1] в Update */
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t Encoder_UKF_Init(Encoder_UKF_t *filter,
                                 const Encoder_UKF_Config_t *config,
                                 float theta_init)
{
    if (filter == NULL) { return SERVO_INVALID; }

    Encoder_UKF_Config_t cfg;
    if (config != NULL) {
        cfg = *config;
    } else {
        cfg.q_theta  = ENCODER_UKF_Q_THETA_DEFAULT;
        cfg.q_omega  = ENCODER_UKF_Q_OMEGA_DEFAULT;
        cfg.q_alpha  = ENCODER_UKF_Q_ALPHA_DEFAULT;
        cfg.r_theta  = ENCODER_UKF_R_THETA_DEFAULT;
        cfg.r_omega  = ENCODER_UKF_R_OMEGA_DEFAULT;
        cfg.p0_theta = ENCODER_UKF_P0_THETA_DEFAULT;
        cfg.p0_omega = ENCODER_UKF_P0_OMEGA_DEFAULT;
        cfg.p0_alpha = ENCODER_UKF_P0_ALPHA_DEFAULT;
    }

    if (ukf_init(&filter->ukf,
                 ENCODER_UKF_N_STATES, ENCODER_UKF_N_MEAS,
                 state_func, meas_func, NULL,
                 filter->buffer) != UKF_SUCCESS) {
        return SERVO_ERROR;
    }
    ukf_set_params(&filter->ukf, 0.5f, 2.0f, 0.0f);

    const float x0[ENCODER_UKF_N_STATES] = { theta_init, 0.0f, 0.0f };
    const float P0[ENCODER_UKF_N_STATES * ENCODER_UKF_N_STATES] = {
        cfg.p0_theta, 0.0f,         0.0f,
        0.0f,         cfg.p0_omega, 0.0f,
        0.0f,         0.0f,         cfg.p0_alpha,
    };
    ukf_set_state(&filter->ukf, x0, P0);

    const float Q[ENCODER_UKF_N_STATES * ENCODER_UKF_N_STATES] = {
        cfg.q_theta, 0.0f,        0.0f,
        0.0f,        cfg.q_omega, 0.0f,
        0.0f,        0.0f,        cfg.q_alpha,
    };
    /* R[1,1] ініціюється як STALE — буде перемикатися в Update */
    const float R[ENCODER_UKF_N_MEAS * ENCODER_UKF_N_MEAS] = {
        cfg.r_theta, 0.0f,
        0.0f,        ENCODER_UKF_R_OMEGA_STALE,
    };
    ukf_set_noise(&filter->ukf, Q, R);

    filter->r_omega     = cfg.r_omega;
    filter->omega_ic    = 0.0f;
    filter->omega_fresh = false;

    return SERVO_OK;
}

Servo_Status_t Encoder_UKF_Update(Encoder_UKF_t *filter, float theta_meas, float dt)
{
    if (filter == NULL) { return SERVO_INVALID; }

    if (ukf_predict(&filter->ukf, dt) != UKF_SUCCESS) { return SERVO_ERROR; }

    /* Перемикаємо R[1,1]: мале при свіжому IC вимірі, велике — ігноруємо */
    ukf_matrix_set(&filter->ukf.R, 1U, 1U,
                   filter->omega_fresh ? filter->r_omega : ENCODER_UKF_R_OMEGA_STALE);

    const float z[ENCODER_UKF_N_MEAS] = { theta_meas, filter->omega_ic };
    if (ukf_update(&filter->ukf, z) != UKF_SUCCESS) { return SERVO_ERROR; }

    filter->omega_fresh = false;
    return SERVO_OK;
}

void Encoder_UKF_FeedOmega(Encoder_UKF_t *filter, float omega_rad_s)
{
    if (filter == NULL) { return; }
    filter->omega_ic    = omega_rad_s;
    filter->omega_fresh = true;
}

void Encoder_UKF_GetState(const Encoder_UKF_t *filter,
                           float *theta, float *omega, float *alpha)
{
    if (filter == NULL) { return; }

    float x[ENCODER_UKF_N_STATES];
    ukf_get_state(&filter->ukf, x, NULL);

    if (theta != NULL) { *theta = x[0]; }
    if (omega != NULL) { *omega = x[1]; }
    if (alpha != NULL) { *alpha = x[2]; }
}

void Encoder_UKF_Reset(Encoder_UKF_t *filter, float theta_init)
{
    if (filter == NULL) { return; }

    const float x0[ENCODER_UKF_N_STATES] = { theta_init, 0.0f, 0.0f };
    const float P0[ENCODER_UKF_N_STATES * ENCODER_UKF_N_STATES] = {
        ENCODER_UKF_P0_THETA_DEFAULT, 0.0f,                        0.0f,
        0.0f,                         ENCODER_UKF_P0_OMEGA_DEFAULT, 0.0f,
        0.0f,                         0.0f,                         ENCODER_UKF_P0_ALPHA_DEFAULT,
    };
    ukf_set_state(&filter->ukf, x0, P0);

    filter->omega_ic    = 0.0f;
    filter->omega_fresh = false;
}
