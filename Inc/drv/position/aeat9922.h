/**
 * @file aeat9922.h
 * @brief Драйвер магнітного енкодера AEAT-9922
 * @author ServoCore Team
 * @date 2025
 *
 * Драйвер для високоточного магнітного енкодера AEAT-9922 від Broadcom.
 * Підтримує абсолютний та інкрементальний режими через SPI інтерфейс.
 * Реалізує Hardware Callbacks Pattern - тільки апаратні операції (SPI),
 * вся логіка (конвертація, velocity, multi-turn) в position.c.
 */

#ifndef SERVOCORE_DRV_AEAT9922_H
#define SERVOCORE_DRV_AEAT9922_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../../core.h"
#include "position.h"
#include "../../hwd/hwd_spi.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/* ========================================================================== */
/* КОНСТАНТИ З DATASHEET                                                      */
/* ========================================================================== */

// Power-Up Time (Table 3, Page 5)
#define AEAT9922_POWERUP_TIME_MS            10      // Час запуску після подачі живлення

// EEPROM Programming
#define AEAT9922_EEPROM_WRITE_TIME_MS       40      // Час запису в EEPROM

// Calibration
#define AEAT9922_CALIB_ACCURACY_TIME_MS     2000    // Час калібрування точності (~2 сек)
#define AEAT9922_CALIB_ZERO_TIME_MS         100     // Час zero reset калібрування

// SPI4 Timing (Table 8, Page 10)
#define AEAT9922_SPI4_T_CSN_MIN_NS          350     // CS LOW до першого SCK
#define AEAT9922_SPI4_T_CSR_MIN_NS          350     // CS HIGH між транзакціями
#define AEAT9922_SPI4_T_CLK_MIN_NS          100     // Clock period (10 MHz max)

// ABI Incremental (Table 5, Page 6)
#define AEAT9922_ABI_REACTION_TIME_MS       10      // Час до першого ABI імпульсу
#define AEAT9922_ABI_MAX_FREQUENCY_HZ       1000000 // Максимальна частота 1 MHz

// Incremental CPR (Table 4, Page 6)
#define AEAT9922_INC_CPR_MIN                1       // Мінімальна CPR
#define AEAT9922_INC_CPR_MAX                10000   // Максимальна CPR

// UVW Pole Pairs (Page 19)
#define AEAT9922_UVW_POLE_PAIRS_MIN         1       // Мінімум 1 пара полюсів
#define AEAT9922_UVW_POLE_PAIRS_MAX         32      // Максимум 32 пари

/* ========================================================================== */
/* РЕЖИМИ РОБОТИ                                                              */
/* ========================================================================== */

/**
 * @brief Прапорці режимів роботи (можна комбінувати)
 */
typedef enum {
    AEAT9922_MODE_NONE             = 0,         // Жоден режим

    // Абсолютні інтерфейси (Table 7, Page 9)
    AEAT9922_MODE_SPI3             = (1 << 0),  // SPI-3 (тільки memory R/W)
    AEAT9922_MODE_SSI3             = (1 << 1),  // SSI-3
    AEAT9922_MODE_SSI2             = (1 << 2),  // SSI-2
    AEAT9922_MODE_SPI4             = (1 << 3),  // SPI-4 (діагностика + position)
    AEAT9922_MODE_PWM              = (1 << 4),  // PWM вихід

    // Інкрементальні виходи
    AEAT9922_MODE_ABI              = (1 << 5),  // ABI інкрементальний вихід
    AEAT9922_MODE_UVW              = (1 << 6),  // UVW комутація
} AEAT9922_Mode_Flags_t;

/**
 * @brief Варіанти протоколів (PSEL[1:0] біти)
 */
typedef enum {
    AEAT9922_PSEL_SPI4_16BIT   = 0,  // PSEL[1:0]=00: SPI-4(A) 16-bit з парністю
    AEAT9922_PSEL_SPI4_24BIT   = 1,  // PSEL[1:0]=01: SPI-4(B) 24-bit з CRC
    AEAT9922_PSEL_SSI3_A       = 0,  // PSEL[1:0]=00: SSI-3(A)
    AEAT9922_PSEL_SSI3_B       = 1,  // PSEL[1:0]=01: SSI-3(B)
    AEAT9922_PSEL_SSI2_A       = 0,  // PSEL[1:0]=00: SSI-2(A)
    AEAT9922_PSEL_SSI2_B       = 1,  // PSEL[1:0]=01: SSI-2(B)
    AEAT9922_PSEL_PWM_SIMPLE   = 0,  // PSEL[1:0]=00: PWM без Init/Error/Exit
    AEAT9922_PSEL_PWM_EXTENDED = 1,  // PSEL[1:0]=01: PWM з Init/Error/Exit
} AEAT9922_Protocol_Variant_t;

/**
 * @brief Роздільна здатність абсолютного виходу (CONFIG1[3:0])
 */
typedef enum {
    AEAT9922_ABS_RES_18BIT = 0,  /**< 262144 позицій */
    AEAT9922_ABS_RES_17BIT = 1,  /**< 131072 позицій */
    AEAT9922_ABS_RES_16BIT = 2,  /**< 65536 позицій */
    AEAT9922_ABS_RES_15BIT = 3,  /**< 32768 позицій */
    AEAT9922_ABS_RES_14BIT = 4,  /**< 16384 позицій */
    AEAT9922_ABS_RES_13BIT = 5,  /**< 8192 позицій */
    AEAT9922_ABS_RES_12BIT = 6,  /**< 4096 позицій */
    AEAT9922_ABS_RES_11BIT = 7,  /**< 2048 позицій */
    AEAT9922_ABS_RES_10BIT = 8   /**< 1024 позицій */
} AEAT9922_Abs_Resolution_t;

/**
 * @brief Index pulse width (регістр CONFIG0)
 */
typedef enum {
    AEAT9922_INDEX_WIDTH_90  = 0,  /**< 90 електричних градусів */
    AEAT9922_INDEX_WIDTH_180 = 1,  /**< 180 електричних градусів */
    AEAT9922_INDEX_WIDTH_270 = 2,  /**< 270 електричних градусів */
    AEAT9922_INDEX_WIDTH_360 = 3   /**< 360 електричних градусів */
} AEAT9922_Index_Width_t;

/**
 * @brief Index state - позиція імпульсу Index (регістр CONFIG0)
 */
typedef enum {
    AEAT9922_INDEX_STATE_90  = 0,  /**< Index на 90° позиції */
    AEAT9922_INDEX_STATE_180 = 1,  /**< Index на 180° позиції */
    AEAT9922_INDEX_STATE_270 = 2,  /**< Index на 270° позиції */
    AEAT9922_INDEX_STATE_360 = 3   /**< Index на 360° позиції (нульова) */
} AEAT9922_Index_State_t;

/**
 * @brief Структура статусу енкодера
 */
typedef struct {
    bool ready;          /**< RDY: Енкодер готовий */
    bool magnet_high;    /**< MHI: Магніт занадто близько */
    bool magnet_low;     /**< MLO: Магніт занадто далеко */
    bool memory_error;   /**< MEM_Err: Помилка пам'яті EEPROM */
} AEAT9922_Status_t;

/**
 * @brief Конфігурація SPI інтерфейсу
 */
typedef struct {
    HWD_SPI_Config_t spi_config;             /**< Апаратна конфігурація SPI */

    void* msel_port;                          /**< Базова адреса GPIO порту для MSEL */
    uint16_t msel_pin;                        /**< Бітова маска піна MSEL */

    AEAT9922_Protocol_Variant_t protocol_variant;  /**< Варіант протоколу (16-bit або 24-bit) */
} AEAT9922_SPI_Config_t;

/**
 * @brief Конфігурація ABI інкрементального виходу
 *
 * Для апаратного підрахунку імпульсів надайте три callbacks:
 *   encoder_start — запускає таймер у режимі лічильника енкодера
 *   encoder_stop  — зупиняє таймер
 *   encoder_read  — повертає поточне значення лічильника (знакове)
 *   encoder_ctx   — довільний контекст (наприклад, базова адреса таймера)
 *
 * Якщо callbacks не потрібні (ABI використовується без апаратного підрахунку),
 * встановіть enable_incremental = false або залиште callbacks = NULL.
 */
typedef struct {
    uint16_t incremental_cpr;                 /**< Counts Per Revolution (1-10000) */

    AEAT9922_Index_Width_t index_width;       /**< Ширина імпульсу Index */
    AEAT9922_Index_State_t index_state;       /**< Позиція імпульсу Index */

    bool enable_incremental;                  /**< Використовувати апаратний підрахунок */

    /** @brief Callback: запуск апаратного таймера у режимі лічильника енкодера */
    Servo_Status_t (*encoder_start)(void* ctx);

    /** @brief Callback: зупинка апаратного таймера */
    Servo_Status_t (*encoder_stop)(void* ctx);

    /** @brief Callback: читання поточного значення лічильника */
    int32_t (*encoder_read)(void* ctx);

    /** @brief Контекст для callbacks (наприклад, базова адреса таймера) */
    void* encoder_ctx;
} AEAT9922_ABI_Config_t;

/**
 * @brief Конфігурація UVW комутаційних сигналів
 */
typedef struct {
    uint8_t pole_pairs;                       /**< Кількість пар полюсів (1-32) */
} AEAT9922_UVW_Config_t;

/**
 * @brief Загальна конфігурація датчика
 */
typedef struct {
    AEAT9922_Abs_Resolution_t abs_resolution; /**< Роздільна здатність (CONFIG1[3:0]) */

    bool direction_ccw;                       /**< true = CCW count up, false = CW count up */

    bool auto_zero_on_init;                   /**< Виконати zero reset при ініціалізації */

    bool enable_inl_correction;               /**< Увімкнути INL angle correction */

    float hysteresis_deg;                     /**< Гістерезис в механічних градусах */
} AEAT9922_General_Config_t;

/**
 * @brief Повна конфігурація AEAT-9922
 */
typedef struct {
    uint32_t enabled_modes;                   /**< Комбінація AEAT9922_Mode_Flags_t */

    AEAT9922_General_Config_t general;        /**< Загальні налаштування */

    AEAT9922_SPI_Config_t spi_config;         /**< Конфігурація SPI (для SPI3, SPI4) */

    AEAT9922_ABI_Config_t abi;                /**< ABI інкрементальний вихід */
    AEAT9922_UVW_Config_t uvw;                /**< UVW комутація */
} AEAT9922_Config_t;

/**
 * @brief Структура драйвера AEAT-9922
 *
 * Містить тільки апаратну специфіку (SPI, статус AEAT).
 * Вся логіка (position, velocity, multi-turn) в Position_Sensor_Interface_t.
 */
typedef struct {
    Position_Sensor_Interface_t interface;  /**< Універсальний інтерфейс (ПЕРШИЙ!) */
    AEAT9922_Config_t config;               /**< Конфігурація енкодера */
    HWD_SPI_Handle_t spi_handle;            /**< Дескриптор SPI */

    // ТІЛЬКИ AEAT-специфічне
    AEAT9922_Status_t status;               /**< Статус енкодера */
    uint32_t error_count;                   /**< Лічильник помилок SPI */

    // Інкрементальний лічильник (якщо використовується)
    int32_t incremental_count;              /**< Загальний інкрементальний лічильник */
    int32_t last_incremental_count;         /**< Попереднє значення лічильника */

} AEAT9922_Driver_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Створення драйвера AEAT-9922
 *
 * Ініціалізує структуру драйвера з конфігурацією та прив'язує hardware callbacks.
 * Після створення використовуйте &driver->interface для доступу до Position_Sensor_Interface_t.
 *
 * @param driver Вказівник на структуру драйвера
 * @param config Вказівник на конфігурацію
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_Create(AEAT9922_Driver_t* driver,
                                const AEAT9922_Config_t* config);

/**
 * @brief Зчитування статусу енкодера
 *
 * Читає регістр статусу та оновлює flags (ready, magnet_high, magnet_low, memory_error)
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_ReadStatus(AEAT9922_Driver_t* driver);

/**
 * @brief Зчитування регістру через SPI
 *
 * @param driver Вказівник на структуру драйвера
 * @param address Адреса регістру (0x00-0x7F)
 * @param value Вказівник для збереження значення
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_ReadRegister(AEAT9922_Driver_t* driver,
                                      uint8_t address, uint8_t* value);

/**
 * @brief Запис регістру через SPI
 *
 * @param driver Вказівник на структуру драйвера
 * @param address Адреса регістру (0x00-0x7F)
 * @param value Значення для запису
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_WriteRegister(AEAT9922_Driver_t* driver,
                                       uint8_t address, uint8_t value);

/**
 * @brief Розблокування регістрів для запису
 *
 * Записує unlock код для дозволу запису в регістри конфігурації
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_UnlockRegisters(AEAT9922_Driver_t* driver);

/**
 * @brief Програмування EEPROM (збереження конфігурації)
 *
 * Зберігає поточну конфігурацію в EEPROM енкодера
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_ProgramEEPROM(AEAT9922_Driver_t* driver);

/**
 * @brief Калібрування точності
 *
 * Виконує automatic accuracy calibration. Вал має обертатися при 60-2000 RPM.
 * Процес займає ~1-2 секунди.
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_CalibrateAccuracy(AEAT9922_Driver_t* driver);

/**
 * @brief Калібрування нульової позиції
 *
 * Встановлює поточну позицію як нульову. Вал має бути нерухомим.
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_CalibrateZero(AEAT9922_Driver_t* driver);

/**
 * @brief Оновлення інкрементального лічильника (якщо використовується)
 *
 * Зчитує значення з апаратного encoder timer та оновлює загальний лічильник
 *
 * @param driver Вказівник на структуру драйвера
 * @return Servo_Status_t Статус виконання
 */
Servo_Status_t AEAT9922_UpdateIncrementalCount(AEAT9922_Driver_t* driver);

/**
 * @brief Callback для Index pulse (якщо використовується)
 *
 * Викликається при спрацьовуванні Index pulse для підрахунку обертів
 *
 * @param driver Вказівник на структуру драйвера
 */
void AEAT9922_IndexPulseCallback(AEAT9922_Driver_t* driver);

/* Адреси регістрів AEAT-9922 -----------------------------------------------*/

#define AEAT9922_REG_CONFIG0            0x07  /**< Customer Configuration-0 */
#define AEAT9922_REG_CONFIG1            0x08  /**< Customer Configuration-1 */
#define AEAT9922_REG_INC_RES_HIGH       0x09  /**< Incremental Resolution High */
#define AEAT9922_REG_INC_RES_LOW        0x0A  /**< Incremental Resolution Low */
#define AEAT9922_REG_CONFIG2            0x0B  /**< Customer Configuration-2 */
#define AEAT9922_REG_ZERO_HIGH          0x0C  /**< Zero Reset High */
#define AEAT9922_REG_ZERO_MID           0x0D  /**< Zero Reset Mid */
#define AEAT9922_REG_ZERO_LOW           0x0E  /**< Zero Reset Low */
#define AEAT9922_REG_UNLOCK             0x10  /**< Unlock Register */
#define AEAT9922_REG_PROGRAM            0x11  /**< Program EEPROM */
#define AEAT9922_REG_CALIBRATE          0x12  /**< Calibration Control */
#define AEAT9922_REG_STATUS             0x21  /**< Status Register */
#define AEAT9922_REG_CALIB_STATUS       0x22  /**< Calibration Status */
#define AEAT9922_REG_POSITION           0x3F  /**< Position Register */

/* Константи ----------------------------------------------------------------*/

#define AEAT9922_UNLOCK_CODE            0xAB  /**< Код розблокування */
#define AEAT9922_PROGRAM_CODE           0xA1  /**< Код програмування EEPROM */
#define AEAT9922_CALIB_ACCURACY_START   0x02  /**< Старт калібрування точності */
#define AEAT9922_CALIB_ZERO_START       0x08  /**< Старт zero reset */
#define AEAT9922_CALIB_EXIT             0x00  /**< Вихід з режиму калібрування */

#define AEAT9922_POWERUP_TIME_MS        10    /**< Час запуску (мс) */
#define AEAT9922_EEPROM_WRITE_TIME_MS   40    /**< Час запису EEPROM (мс) */
#define AEAT9922_CALIB_TIME_MS          2000  /**< Час калібрування (мс) */

#ifdef __cplusplus
}
#endif

#endif /* SERVOCORE_DRV_AEAT9922_H */
