/*******************************************************************************
 * @file    hwd_eip.h
 * @brief   Hardware Driver Layer для EtherNet/IP communication
 * @details OpENer-based EtherNet/IP stack для I/O adapter devices
 *          Підтримка DS402 servo drive profile
 *
 * @note    Compile-time enable: #define USE_HWD_EIP
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#ifndef HWD_EIP_H
#define HWD_EIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config_lib.h"
#include "err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef USE_HWD_EIP

/*******************************************************************************
 * DEFINES
 ******************************************************************************/

/** EtherNet/IP standard ports */
#define HWD_EIP_EXPLICIT_PORT       44818   /**< TCP port для Explicit Messaging */
#define HWD_EIP_IMPLICIT_PORT       2222    /**< UDP port для Implicit I/O */

/** Assembly IDs */
#define HWD_EIP_INPUT_ASSEMBLY_ID   100     /**< Input Assembly (Telemetry) */
#define HWD_EIP_OUTPUT_ASSEMBLY_ID  150     /**< Output Assembly (Commands) */
#define HWD_EIP_CONFIG_ASSEMBLY_ID  151     /**< Config Assembly (unused) */

/** Watchdog timeout */
#define HWD_EIP_WATCHDOG_TIMEOUT_MS 100     /**< Connection timeout (ms) */

/** Command Assembly size */
#define HWD_EIP_COMMAND_SIZE        8       /**< Fixed size: 8 bytes */

/** Maximum telemetry size */
#define HWD_EIP_TELEMETRY_MAX_SIZE  32      /**< Max size: 32 bytes */

/*******************************************************************************
 * TYPES - DS402 Control/Status Words
 ******************************************************************************/

/** DS402 Control Word bits */
typedef enum {
    HWD_EIP_CW_SWITCH_ON        = (1 << 0),  /**< Switch On */
    HWD_EIP_CW_ENABLE_VOLTAGE   = (1 << 1),  /**< Enable Voltage */
    HWD_EIP_CW_QUICK_STOP       = (1 << 2),  /**< Quick Stop (1=normal) */
    HWD_EIP_CW_ENABLE_OPERATION = (1 << 3),  /**< Enable Operation */
    HWD_EIP_CW_FAULT_RESET      = (1 << 7),  /**< Fault Reset */
    HWD_EIP_CW_HALT             = (1 << 8)   /**< Halt */
} HWD_EIP_ControlWord_Bits_t;

/** DS402 Status Word bits */
typedef enum {
    HWD_EIP_SW_READY_TO_SWITCH_ON = (1 << 0),  /**< Ready to Switch On */
    HWD_EIP_SW_SWITCHED_ON        = (1 << 1),  /**< Switched On */
    HWD_EIP_SW_OPERATION_ENABLED  = (1 << 2),  /**< Operation Enabled */
    HWD_EIP_SW_FAULT              = (1 << 3),  /**< Fault */
    HWD_EIP_SW_VOLTAGE_ENABLED    = (1 << 4),  /**< Voltage Enabled */
    HWD_EIP_SW_QUICK_STOP         = (1 << 5),  /**< Quick Stop (0=active) */
    HWD_EIP_SW_SWITCH_ON_DISABLED = (1 << 6),  /**< Switch On Disabled */
    HWD_EIP_SW_WARNING            = (1 << 9),  /**< Warning */
    HWD_EIP_SW_TARGET_REACHED     = (1 << 10), /**< Target Reached */
    HWD_EIP_SW_MOTOR_STALLED      = (1 << 12), /**< Motor Stalled (custom) */
    HWD_EIP_SW_OVERCURRENT        = (1 << 13)  /**< Overcurrent (custom) */
} HWD_EIP_StatusWord_Bits_t;

/** DS402 Mode of Operation */
typedef enum {
    HWD_EIP_MODE_NO_MODE              = 0,    /**< No mode */
    HWD_EIP_MODE_PROFILE_POSITION     = 1,    /**< Profile Position (future) */
    HWD_EIP_MODE_VELOCITY             = 3,    /**< Velocity (future) */
    HWD_EIP_MODE_PROFILE_VELOCITY     = 3,    /**< Profile Velocity (future) */
    HWD_EIP_MODE_TORQUE               = 4,    /**< Torque (future) */
    HWD_EIP_MODE_HOMING               = 6,    /**< Homing (future) */
    HWD_EIP_MODE_INTERPOLATED_POS     = 7,    /**< Interpolated Position (future) */
    HWD_EIP_MODE_CYCLIC_SYNC_POSITION = 8,    /**< Cyclic Sync Position (future) */
    HWD_EIP_MODE_CYCLIC_SYNC_VELOCITY = 9,    /**< Cyclic Sync Velocity (future) */
    HWD_EIP_MODE_CYCLIC_SYNC_TORQUE   = 10,   /**< Cyclic Sync Torque (future) */
    HWD_EIP_MODE_PWM_DIRECT           = 128   /**< Direct PWM control (current) */
} HWD_EIP_ModeOfOperation_t;

/** Brake command */
typedef enum {
    HWD_EIP_BRAKE_NO_ACTION = 0,   /**< No action */
    HWD_EIP_BRAKE_RELEASE   = 1,   /**< Release brake */
    HWD_EIP_BRAKE_ENGAGE    = 2    /**< Engage brake */
} HWD_EIP_BrakeCommand_t;

/*******************************************************************************
 * TYPES - Assembly Structures
 ******************************************************************************/

/**
 * @brief Output Assembly (Commands: Controller → ServoLib)
 * @note Fixed size: 8 bytes
 */
typedef struct __attribute__((packed)) {
    uint16_t control_word;      /**< DS402 Control Word */
    float    pwm_duty_percent;  /**< PWM duty cycle: -100.0 to +100.0 */
    uint8_t  mode_of_operation; /**< Mode of operation (DS402) */
    uint8_t  brake_command;     /**< Brake command */
} HWD_EIP_Command_t;

/**
 * @brief Input Assembly Header (Telemetry: ServoLib → Controller)
 * @note Always present, 8 bytes
 */
typedef struct __attribute__((packed)) {
    uint16_t status_word;       /**< DS402 Status Word */
    uint16_t warning_code;      /**< Warning flags */
    uint32_t error_code;        /**< Error code */
} HWD_EIP_Telemetry_Header_t;

/**
 * @brief Full telemetry structure (динамічний розмір)
 * @note Actual size залежить від enabled sensors
 */
typedef struct {
    HWD_EIP_Telemetry_Header_t header;  /**< Header (8 bytes) */

    /* Опціональні поля (додаються динамічно) */
    int32_t  position_counts;           /**< Encoder position (4 bytes) */
    float    velocity_rad_s;            /**< Velocity (4 bytes) */
    float    motor_current_A;           /**< Current (4 bytes) */
    float    motor_voltage_V;           /**< Voltage (4 bytes) */
    float    winding_temp_C;            /**< Temperature (4 bytes) */
} HWD_EIP_Telemetry_Full_t;

/*******************************************************************************
 * TYPES - Configuration
 ******************************************************************************/

/**
 * @brief EtherNet/IP Identity Object configuration
 */
typedef struct {
    uint16_t vendor_id;         /**< Vendor ID (65535 для experimental) */
    uint16_t device_type;       /**< Device Type (0x02 = AC Drive) */
    uint16_t product_code;      /**< Product Code */
    uint32_t serial_number;     /**< Serial Number (unique) */
    uint8_t  revision_major;    /**< Major revision */
    uint8_t  revision_minor;    /**< Minor revision */
    char     product_name[32];  /**< Product name string */
} HWD_EIP_Identity_t;

/**
 * @brief Sensor enable flags для динамічного assembly
 */
typedef struct {
    bool position_enabled;      /**< Position sensor enabled */
    bool velocity_enabled;      /**< Velocity calculation enabled */
    bool current_enabled;       /**< Current sensor enabled */
    bool voltage_enabled;       /**< Voltage sensor enabled */
    bool temperature_enabled;   /**< Temperature sensor enabled */
} HWD_EIP_SensorConfig_t;

/**
 * @brief EtherNet/IP driver configuration
 */
typedef struct {
    HWD_EIP_Identity_t identity;        /**< Device identity */
    HWD_EIP_SensorConfig_t sensors;     /**< Sensor configuration */
    uint16_t explicit_port;             /**< TCP port (default: 44818) */
    uint16_t implicit_port;             /**< UDP port (default: 2222) */
    uint16_t watchdog_timeout_ms;       /**< Watchdog timeout (default: 100ms) */
} HWD_EIP_Config_t;

/**
 * @brief EtherNet/IP driver handle
 */
typedef struct {
    HWD_EIP_Config_t config;            /**< Configuration */
    bool initialized;                   /**< Initialization flag */
    bool connection_active;             /**< Connection active flag */
    uint32_t last_command_time_ms;      /**< Last command timestamp */
    uint32_t timeout_count;             /**< Timeout counter */
    uint16_t telemetry_size;            /**< Current telemetry size (bytes) */
} HWD_EIP_Handle_t;

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * @brief Ініціалізація EtherNet/IP driver
 *
 * @param handle    Вказівник на handle структуру
 * @param config    Вказівник на конфігурацію
 * @return Servo_Status_t
 *         - SERVO_OK: успішна ініціалізація
 *         - SERVO_ERROR: помилка ініціалізації
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_Init(HWD_EIP_Handle_t* handle, const HWD_EIP_Config_t* config);

/**
 * @brief Оновлення EtherNet/IP (викликати кожен цикл @ 1 kHz)
 *
 * @param handle    Вказівник на handle структуру
 * @return Servo_Status_t
 *         - SERVO_OK: успішне оновлення
 *         - SERVO_TIMEOUT: watchdog timeout
 *         - SERVO_ERROR: помилка обробки
 */
Servo_Status_t HWD_EIP_Update(HWD_EIP_Handle_t* handle);

/**
 * @brief Отримати останню отриману команду
 *
 * @param handle    Вказівник на handle структуру
 * @param command   Вказівник для запису команди
 * @return Servo_Status_t
 *         - SERVO_OK: команда отримана
 *         - SERVO_ERROR: немає нової команди
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_GetCommand(HWD_EIP_Handle_t* handle, HWD_EIP_Command_t* command);

/**
 * @brief Встановити телеметрію для відправки
 *
 * @param handle        Вказівник на handle структуру
 * @param header        Вказівник на header (завжди потрібен)
 * @param position      Вказівник на position (NULL якщо disabled)
 * @param velocity      Вказівник на velocity (NULL якщо disabled)
 * @param current       Вказівник на current (NULL якщо disabled)
 * @param voltage       Вказівник на voltage (NULL якщо disabled)
 * @param temperature   Вказівник на temperature (NULL якщо disabled)
 * @return Servo_Status_t
 *         - SERVO_OK: телеметрія встановлена
 *         - SERVO_ERROR_NULL_PTR: null pointer для header
 */
Servo_Status_t HWD_EIP_SetTelemetry(
    HWD_EIP_Handle_t* handle,
    const HWD_EIP_Telemetry_Header_t* header,
    const int32_t* position,
    const float* velocity,
    const float* current,
    const float* voltage,
    const float* temperature
);

/**
 * @brief Перевірка watchdog timeout
 *
 * @param handle    Вказівник на handle структуру
 * @return bool
 *         - true: timeout виявлено (потрібен emergency stop)
 *         - false: з'єднання активне
 */
bool HWD_EIP_IsWatchdogTimeout(HWD_EIP_Handle_t* handle);

/**
 * @brief Скидання watchdog після emergency stop
 *
 * @param handle    Вказівник на handle структуру
 */
void HWD_EIP_ResetWatchdog(HWD_EIP_Handle_t* handle);

/**
 * @brief Shutdown EtherNet/IP driver
 *
 * @param handle    Вказівник на handle структуру
 */
void HWD_EIP_Shutdown(HWD_EIP_Handle_t* handle);

/**
 * @brief Отримати поточний розмір телеметрії
 *
 * @param handle    Вказівник на handle структуру
 * @return uint16_t Розмір у байтах
 */
uint16_t HWD_EIP_GetTelemetrySize(const HWD_EIP_Handle_t* handle);

/**
 * @brief Перевірка чи з'єднання активне
 *
 * @param handle    Вказівник на handle структуру
 * @return bool     true якщо з'єднання активне
 */
bool HWD_EIP_IsConnected(const HWD_EIP_Handle_t* handle);

#endif /* USE_HWD_EIP */

#ifdef __cplusplus
}
#endif

#endif /* HWD_EIP_H */
