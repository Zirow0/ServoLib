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

/**
 * @brief Режими інтерфейсу AEAT-9922
 */
typedef enum {
    AEAT9922_INTERFACE_SPI4_16BIT = 0,  /**< SPI-4(A) 16-біт з парністю */
    AEAT9922_INTERFACE_SPI4_24BIT = 1   /**< SPI-4(B) 24-біт з CRC */
} AEAT9922_Interface_t;

/**
 * @brief Роздільна здатність абсолютного виходу
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
 * @brief Структура статусу енкодера
 */
typedef struct {
    bool ready;          /**< RDY: Енкодер готовий */
    bool magnet_high;    /**< MHI: Магніт занадто близько */
    bool magnet_low;     /**< MLO: Магніт занадто далеко */
    bool memory_error;   /**< MEM_Err: Помилка пам'яті EEPROM */
} AEAT9922_Status_t;

/**
 * @brief Конфігурація AEAT-9922
 */
typedef struct {
    // SPI конфігурація
    HWD_SPI_Config_t spi_config;

    // GPIO для MSEL піна
    void* msel_port;      /**< GPIO порт для MSEL */
    uint16_t msel_pin;    /**< GPIO пін для MSEL */

    // Налаштування роздільної здатності
    AEAT9922_Abs_Resolution_t abs_resolution;  /**< Абсолютна роздільність */
    uint16_t incremental_cpr;                   /**< Інкрементальна CPR (1-10000) */

    // Інтерфейс
    AEAT9922_Interface_t interface_mode;

    // Напрямок обертання (true = CCW count up)
    bool direction_ccw;

    // Параметри інкрементального виходу (опціонально)
    bool enable_incremental;      /**< Використовувати апаратний підрахунок */
    void* encoder_timer_handle;   /**< Вказівник на TIM handle для encoder mode */

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
