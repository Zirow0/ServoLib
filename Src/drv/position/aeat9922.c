/**
 * @file aeat9922.c
 * @brief Реалізація драйвера магнітного енкодера AEAT-9922
 * @author ServoCore Team
 * @date 2025
 *
 * Hardware Callbacks Pattern: тільки апаратні операції (SPI read/write).
 * Вся логіка (конвертація raw→degrees, velocity, multi-turn) в position.c.
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_AEAT9922

/* Auto-enable dependencies */
#ifndef USE_SENSOR_POSITION
	#define USE_SENSOR_POSITION
#endif

#include "drv/position/aeat9922.h"
#include "hwd/hwd_timer.h"
#include "hwd/hwd_gpio.h"
#include "util/checksum.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

// Макроси для SPI команд
#define AEAT9922_CMD_READ(addr)   (0x40)           // Read command: RW=1 (bit 6)
#define AEAT9922_CMD_WRITE(addr)  (0x00)           // Write command: RW=0

// Біти статусу
#define AEAT9922_STATUS_RDY_BIT       (1 << 7)
#define AEAT9922_STATUS_MHI_BIT       (1 << 6)
#define AEAT9922_STATUS_MLO_BIT       (1 << 5)
#define AEAT9922_STATUS_MEMERR_BIT    (1 << 4)

// Біти калібрування
#define AEAT9922_CALIB_ACC_PASS       (0x02)
#define AEAT9922_CALIB_ACC_FAIL       (0x03)
#define AEAT9922_CALIB_ZERO_PASS      (0x02 << 2)
#define AEAT9922_CALIB_ZERO_FAIL      (0x03 << 2)

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief Мікросекундна затримка (приблизна, для SPI timing)
 */
static inline void delay_us(uint32_t us)
{
    // Приблизно 25 циклів на мікросекунду при 100 MHz CPU
    volatile uint32_t cycles = us * 25;
    while (cycles--);
}

/* Private hardware callbacks ------------------------------------------------*/

/**
 * @brief Hardware Init Callback
 */
static Servo_Status_t AEAT9922_HW_Init(void* driver_data, const Position_Params_t* params)
{
    AEAT9922_Driver_t* driver = (AEAT9922_Driver_t*)driver_data;
    Servo_Status_t status;
    uint32_t modes = driver->config.enabled_modes;

    // 1. Встановити MSEL відповідно до режимів
    // MSEL=1 для SPI4/PWM/UVW, MSEL=0 для SPI3/SSI
    HWD_GPIO_PinState_t msel_state = HWD_GPIO_PIN_RESET;
    if (modes & (AEAT9922_MODE_SPI4 | AEAT9922_MODE_PWM | AEAT9922_MODE_UVW)) {
        msel_state = HWD_GPIO_PIN_SET;
    }
    HWD_GPIO_WritePin(driver->config.spi_config.msel_port,
                      driver->config.spi_config.msel_pin, msel_state);

    // 2. Ініціалізація SPI (якщо використовується будь-який SPI режим)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        status = HWD_SPI_Init(&driver->spi_handle, &driver->config.spi_config.spi_config);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // 3. Зачекати Power-Up час (10 ms)
    HWD_Timer_DelayMs(AEAT9922_POWERUP_TIME_MS);

    // 4. Перевірити статус енкодера (якщо SPI доступний)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        status = AEAT9922_ReadStatus(driver);
        if (status != SERVO_OK) {
            return status;
        }

        if (!driver->status.ready) {
            return SERVO_ERROR;
        }
    }

    // 5. Прочитати та перевірити конфігурацію CONFIG1 (роздільність, напрямок)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        uint8_t config1;
        status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CONFIG1, &config1);
        if (status != SERVO_OK) {
            return status;
        }

        // Витягнути поточну роздільність (біти [3:0])
        uint8_t current_resolution = config1 & 0x0F;

        // Витягнути напрямок (біт 4)
        bool current_direction_ccw = (config1 & (1 << 4)) != 0;

        // Перевірка: чи конфігурація датчика відповідає очікуваній
        if (current_resolution != (driver->config.general.abs_resolution & 0x0F)) {
            // Попередження: роздільність не відповідає конфігурації
            // Можна логувати або повернути помилку
        }

        if (current_direction_ccw != driver->config.general.direction_ccw) {
            // Попередження: напрямок не відповідає конфігурації
        }

        // Роздільність перевірена, конвертація буде в HW_ReadRaw
    }

    // 6. Прочитати та перевірити інкрементальну роздільність (якщо режим ABI)
    if (modes & AEAT9922_MODE_ABI) {
        uint8_t cpr_high, cpr_low;

        status = AEAT9922_ReadRegister(driver, AEAT9922_REG_INC_RES_HIGH, &cpr_high);
        if (status != SERVO_OK) {
            return status;
        }

        status = AEAT9922_ReadRegister(driver, AEAT9922_REG_INC_RES_LOW, &cpr_low);
        if (status != SERVO_OK) {
            return status;
        }

        uint16_t current_cpr = ((uint16_t)(cpr_high & 0x3F) << 8) | cpr_low;

        // Перевірка: чи CPR відповідає конфігурації
        if (current_cpr != driver->config.abi.incremental_cpr) {
            // Попередження: CPR не відповідає конфігурації
        }
    }

    // 7. Прочитати та перевірити CONFIG2 (PSEL - варіант протоколу)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        uint8_t config2;
        status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CONFIG2, &config2);
        if (status != SERVO_OK) {
            return status;
        }

        // Витягнути PSEL[1:0] з бітів [6:5]
        uint8_t current_psel = (config2 >> 5) & 0x03;

        // Перевірка: чи PSEL відповідає конфігурації
        if (current_psel != (driver->config.spi_config.protocol_variant & 0x03)) {
            // Попередження: варіант протоколу не відповідає конфігурації
        }
    }

    // 8. Ініціалізувати апаратний таймер для ABI (якщо використовується)
    if ((modes & AEAT9922_MODE_ABI) && driver->config.abi.enable_incremental) {
        if (driver->config.abi.encoder_start != NULL) {
            driver->config.abi.encoder_start(driver->config.abi.encoder_ctx);
        }
        driver->incremental_count = 0;
        driver->last_incremental_count = 0;
    }

    // 9. Перевірка готовності датчика до роботи
    // Всі регістри прочитані, конфігурація перевірена
    // Датчик готовий до використання

    return SERVO_OK;
}

/**
 * @brief Hardware DeInit Callback
 */
static Servo_Status_t AEAT9922_HW_DeInit(void* driver_data)
{
    AEAT9922_Driver_t* driver = (AEAT9922_Driver_t*)driver_data;
    uint32_t modes = driver->config.enabled_modes;

    // Зупинити інкрементальний таймер (якщо ABI увімкнений)
    if ((modes & AEAT9922_MODE_ABI) && driver->config.abi.enable_incremental) {
        if (driver->config.abi.encoder_stop != NULL) {
            driver->config.abi.encoder_stop(driver->config.abi.encoder_ctx);
        }
    }

    // Деініціалізувати SPI (якщо використовується)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        return HWD_SPI_DeInit(&driver->spi_handle);
    }

    return SERVO_OK;
}

/**
 * @brief Hardware Read Raw Callback (КЛЮЧОВА ФУНКЦІЯ!)
 *
 * Читає ТІЛЬКИ сирі дані через SPI, БЕЗ конвертації в градуси.
 * Конвертацію робить position.c.
 *
 * Підтримувані протоколи:
 *
 * SPI4-A Protocol (16-bit з парністю):
 * TX: [CMD | ADDR]
 * RX: [P|EF|0|0|0|0|Pos[9:8]] [Pos[7:0]]
 *
 * SPI4-B Protocol (24-bit з CRC):
 * TX: [CMD | ADDR | CRC]
 * RX: [Reserved(4)|W|E|Data[17:16]] [Data[15:8]] [CRC-8]
 */
static Servo_Status_t AEAT9922_HW_ReadRaw(void* driver_data, Position_Raw_Data_t* raw)
{
    AEAT9922_Driver_t* driver = (AEAT9922_Driver_t*)driver_data;
    Servo_Status_t status;
    uint8_t tx_data[3];
    uint8_t rx_data[3] = {0};
    uint8_t packet_size;

    // Визначити розмір пакету залежно від протоколу
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        packet_size = 3;  // SPI4-B: 24-bit з CRC
    } else {
        packet_size = 2;  // SPI4-A: 16-bit з парністю
    }

    // Відправити команду читання позиції (register 0x3F)
    tx_data[0] = AEAT9922_CMD_READ(AEAT9922_REG_POSITION);  // 0x40
    tx_data[1] = AEAT9922_REG_POSITION;                     // 0x3F

    // Розрахувати CRC для TX пакету (тільки для SPI4-B)
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        tx_data[2] = Checksum_CRC8(tx_data, 2, CRC8_POLY_DEFAULT);
    } else {
        tx_data[2] = 0x00;  // Не використовується для SPI4-A
    }

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, packet_size);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    delay_us(1);  // t_CSn >= 350ns
    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    tx_data[0] = 0x00;
    tx_data[1] = 0x00;
    tx_data[2] = 0x00;

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, packet_size);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        driver->error_count++;
        raw->valid = false;
        return status;
    }

    // Розпакувати дані залежно від протоколу
    uint32_t raw_position;
    bool error_flag = false;

    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_16BIT) {
        /* ====================================================================
         * SPI4-A РОЗПАКОВКА (16-bit з парністю)
         * ====================================================================
         * Формат відповіді:
         * Байт 0 [15:8]: P | EF | 0 | 0 | 0 | 0 | Pos[9:8]
         * Байт 1 [7:0]:  Pos[7:0]
         *
         * Біти:
         * [15]    - P (Parity): even parity біт
         * [14]    - EF (Error Flag): помилка
         * [13:10] - Reserved (завжди 0)
         * [9:0]   - Position data (10 біт)
         */

        // Перевірка парності (even parity)
        uint16_t full_data = ((uint16_t)rx_data[0] << 8) | rx_data[1];
        uint8_t parity_check = Checksum_EvenParity16(full_data);

        if (parity_check != 0) {
            // Помилка парності (парність повинна бути парною, тобто 0)
            driver->error_count++;
            raw->valid = false;
            return SERVO_ERROR;
        }

        // Витягнути прапорець помилки (біт 14)
        error_flag = (rx_data[0] & (1 << 6)) != 0;  // EF bit

        // if (error_flag) {
        //     driver->error_count++;
        //     raw->valid = false;
        //     return SERVO_ERROR;
        // }

        // Витягнути 10-bit позицію (біти [9:0])
        raw_position = ((uint32_t)(full_data & 0x3fff));  // Біти [13:0]

    } else {
        /* ====================================================================
         * SPI4-B РОЗПАКОВКА (24-bit з парністю)
         * ====================================================================
         * Формат відповіді (підтверджено логічним аналізатором):
         * Байт 0 [23:16]: P | W | E | Pos[17:13]
         * Байт 1 [15:8]:  Pos[12:5]
         * Байт 2 [7:0]:   Pos[4:0] | pad(3)
         *
         * P   = bit 23 — парність (even parity над Байт0[6:0]+Байт1+Байт2)
         * W   = bit 22 — Warning (магніт поза оптимальним діапазоном)
         * E   = bit 21 — Error (критична помилка)
         * Pos = 18 біт позиції в бітах [20:3]
         */

        // Перевірка парності: P = even parity над BYTE0[6:0] + BYTE1 + BYTE2
        uint8_t received_parity = (rx_data[0] >> 7) & 1;
        uint16_t w0 = ((uint16_t)(rx_data[0] & 0x7F) << 8) | rx_data[1];
        uint8_t calc_parity = Checksum_EvenParity16(w0) ^
                              Checksum_EvenParity16((uint16_t)rx_data[2]);

        if (calc_parity != received_parity) {
            driver->error_count++;
            raw->valid = false;
            return SERVO_ERROR;
        }

        bool warning_flag = (rx_data[0] & 0x40) != 0;  // W = bit 6
        error_flag        = (rx_data[0] & 0x20) != 0;  // E = bit 5

        if (error_flag) {
            driver->error_count++;
            raw->valid = false;
            return SERVO_ERROR;
        }

        (void)warning_flag;  // Продовжуємо навіть при попередженні

        // Витягнути 18-bit позицію
        uint32_t position_18bit = ((uint32_t)(rx_data[0] & 0x1F) << 13) |  // Pos[17:13]
                                  ((uint32_t)rx_data[1] << 5)              |  // Pos[12:5]
                                  ((rx_data[2] >> 3) & 0x1F);                // Pos[4:0]

        raw_position = position_18bit;
    }

    // Застосувати маску відповідно до налаштованої роздільності
    uint32_t resolution_bits = 18 - (uint8_t)driver->config.general.abs_resolution;
    uint32_t position_mask = (1U << resolution_bits) - 1;
    raw_position = raw_position & position_mask;

    // ========== КОНВЕРТАЦІЯ RAW → RADIANS ==========
    // Максимальне значення для даної роздільності
    uint32_t max_count = (1U << resolution_bits);

    // Конвертувати в радіани (0-2π)
    float angle_rad = ((float)raw_position * TWO_PI) / (float)max_count;

    // Заповнити структуру Position_Raw_Data_t
    raw->angle_rad = angle_rad;             // float, 0-2π
    raw->timestamp_us = HWD_Timer_GetMicros();
    raw->has_velocity = false;              // AEAT-9922 НЕ надає готову velocity
    raw->velocity_rad_s = 0.0f;
    raw->valid = true;

    return SERVO_OK;
}

/**
 * @brief Hardware Calibrate Callback
 */
static Servo_Status_t AEAT9922_HW_Calibrate(void* driver_data)
{
    AEAT9922_Driver_t* driver = (AEAT9922_Driver_t*)driver_data;

    // Викликати zero reset (калібрування нульової позиції)
    return AEAT9922_CalibrateZero(driver);
}

/* Exported functions --------------------------------------------------------*/

Servo_Status_t AEAT9922_Create(AEAT9922_Driver_t* driver,
                                const AEAT9922_Config_t* config)
{
    if (driver == NULL || config == NULL) {
        return SERVO_INVALID;
    }

    // Ініціалізація структури
    memset(driver, 0, sizeof(AEAT9922_Driver_t));
    memcpy(&driver->config, config, sizeof(AEAT9922_Config_t));

    // Прив'язати Hardware Callbacks
    driver->interface.hw.init = AEAT9922_HW_Init;
    driver->interface.hw.deinit = AEAT9922_HW_DeInit;
    driver->interface.hw.read_raw = AEAT9922_HW_ReadRaw;
    driver->interface.hw.calibrate = AEAT9922_HW_Calibrate;
    driver->interface.hw.notify_callback = NULL;  // Не використовується

    // Налаштувати метадані інтерфейсу
    driver->interface.capabilities = POSITION_CAP_ABSOLUTE | POSITION_CAP_MULTITURN;
    driver->interface.requires_calibration = false;  // Абсолютний енкодер
    driver->interface.driver_data = driver;

    return SERVO_OK;
}

Servo_Status_t AEAT9922_ReadStatus(AEAT9922_Driver_t* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    uint8_t status_reg;
    Servo_Status_t status = AEAT9922_ReadRegister(driver,
                                                   AEAT9922_REG_STATUS,
                                                   &status_reg);
    if (status != SERVO_OK) {
        return status;
    }

    driver->status.ready = (status_reg & AEAT9922_STATUS_RDY_BIT) != 0;
    driver->status.magnet_high = (status_reg & AEAT9922_STATUS_MHI_BIT) != 0;
    driver->status.magnet_low = (status_reg & AEAT9922_STATUS_MLO_BIT) != 0;
    driver->status.memory_error = (status_reg & AEAT9922_STATUS_MEMERR_BIT) != 0;

    return SERVO_OK;
}

Servo_Status_t AEAT9922_ReadRegister(AEAT9922_Driver_t* driver,
                                      uint8_t address, uint8_t* value)
{
    if (driver == NULL || value == NULL) {
        return SERVO_INVALID;
    }

    Servo_Status_t status;

    /* ========================================================================
     * SPI4-B READ REGISTER PROTOCOL
     * ========================================================================
     * Транзакція 1 (команда):
     *   TX: [CMD_READ | ADDRESS | DUMMY]
     *   RX: [ignored]
     *
     * Транзакція 2 (дані):
     *   TX: [DUMMY | DUMMY | DUMMY]
     *   RX: [DATA | Reserved | CRC-8] для SPI4-B
     *   RX: [DATA | Reserved] для SPI3
     */

    // Визначити розмір пакету залежно від режиму
    uint8_t packet_size = 2;  // За замовчуванням SPI3 та SPI4-A
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        packet_size = 3;  // SPI4-B з CRC
    }

    uint8_t tx_data[3] = {0};
    uint8_t rx_data[3] = {0};

    // Крок 1: Відправити команду читання (RW=1, адреса)
    tx_data[0] = AEAT9922_CMD_READ(address);  // 0x40 (RW=1)
    tx_data[1] = address;                     // Адреса регістру

    // Розрахувати CRC для TX пакету (тільки для SPI4-B)
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        tx_data[2] = Checksum_CRC8(tx_data, 2, CRC8_POLY_DEFAULT);
    } else {
        tx_data[2] = 0x00;  // Не використовується для SPI3/SPI4-A
    }

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, packet_size);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        return status;
    }

    delay_us(2);  // t_CSR >= 350ns (між транзакціями)

    // Крок 2: Зчитати дані в наступній транзакції
    tx_data[0] = 0x00;  // Dummy
    tx_data[1] = 0x00;  // Dummy
    tx_data[2] = 0x00;  // Dummy

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, packet_size);

    delay_us(1);
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        return status;
    }

    /*
     * SPI4-B фрейм відповіді для 8-bit регістрів (підтверджено логічним аналізатором):
     *   rx_data[0] = значення регістру (DATA)
     *   rx_data[1] = W(1) | E(1) | CRC6(6)
     *   rx_data[2] = 0x00 (паддінг)
     *
     * CRC-6: polynomial=0x04, init=0x04, MSB-first, над rx_data[0] (1 байт).
     * W=bit7 (Warning: магніт не в оптимальному діапазоні)
     * E=bit6 (Error: критична помилка)
     */
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        uint8_t calculated_crc = Checksum_CRC6(&rx_data[0], 1, CRC6_POLY_AEAT9922);
        uint8_t received_crc   = rx_data[1] & 0x3F;

        if (calculated_crc != received_crc) {
            driver->error_count++;
            return SERVO_ERROR;
        }

        bool error_bit = (rx_data[1] & 0x40) != 0;  // E = bit 6
        if (error_bit) {
            driver->error_count++;
            return SERVO_ERROR;
        }
    }

    // Перевірка парності для SPI4-A (16-bit режим з парністю)
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_16BIT) {
        uint16_t full_data = ((uint16_t)rx_data[0] << 8) | rx_data[1];
        uint8_t parity_check = Checksum_EvenParity16(full_data);

        if (parity_check != 0) {
            return SERVO_ERROR;
        }
    }

    // Дані знаходяться в першому байті відповіді
    *value = rx_data[0];

    return SERVO_OK;
}

Servo_Status_t AEAT9922_WriteRegister(AEAT9922_Driver_t* driver,
                                       uint8_t address, uint8_t value)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    /* ========================================================================
     * SPI4-B WRITE REGISTER PROTOCOL
     * ========================================================================
     * Для SPI4-B (24-bit з CRC):
     *   TX: [CMD_WRITE | ADDRESS | DATA]
     *   Потім в наступній транзакції надсилається CRC
     *
     * Для SPI3:
     *   TX: [CMD_WRITE | ADDRESS | DATA]
     */

    // Визначити розмір пакету
    uint8_t packet_size = 3;  // Завжди 3 байти для write

    uint8_t tx_data[3];
    uint8_t rx_data[3] = {0};

    // Команда запису: RW=0 (bit 6 = 0), адреса, дані
    tx_data[0] = AEAT9922_CMD_WRITE(address);  // 0x00 (RW=0)
    tx_data[1] = address;                       // Адреса регістру
    tx_data[2] = value;                         // Дані для запису

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    Servo_Status_t status = HWD_SPI_TransmitReceive(&driver->spi_handle,
                                                     tx_data, rx_data, packet_size);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        return status;
    }

    // Для SPI4-B надіслати CRC в окремій транзакції
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        delay_us(2);  // t_CSR >= 350ns

        // Розрахувати CRC для відправлених даних
        uint8_t crc = Checksum_CRC8(tx_data, 3, CRC8_POLY_DEFAULT);

        uint8_t crc_tx[1] = {crc};
        uint8_t crc_rx[1] = {0};

        HWD_SPI_CS_Low(&driver->spi_handle);
        delay_us(1);

        status = HWD_SPI_TransmitReceive(&driver->spi_handle, crc_tx, crc_rx, 1);

        delay_us(1);
        HWD_SPI_CS_High(&driver->spi_handle);

        if (status != SERVO_OK) {
            return status;
        }
    }

    // Час на обробку запису (мінімум 1 ms)
    HWD_Timer_DelayMs(1);

    return SERVO_OK;
}

Servo_Status_t AEAT9922_UnlockRegisters(AEAT9922_Driver_t* driver)
{
    return AEAT9922_WriteRegister(driver, AEAT9922_REG_UNLOCK,
                                   AEAT9922_UNLOCK_CODE);
}

Servo_Status_t AEAT9922_ProgramEEPROM(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Записати код програмування
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_PROGRAM,
                                     AEAT9922_PROGRAM_CODE);
    if (status != SERVO_OK) {
        return status;
    }

    // Зачекати завершення (40 ms)
    HWD_Timer_DelayMs(AEAT9922_EEPROM_WRITE_TIME_MS);

    // Перевірити статус пам'яті
    status = AEAT9922_ReadStatus(driver);
    if (status != SERVO_OK) {
        return status;
    }

    if (driver->status.memory_error) {
        return SERVO_ERROR;
    }

    return SERVO_OK;
}

Servo_Status_t AEAT9922_CalibrateAccuracy(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Розблокувати регістри
    status = AEAT9922_UnlockRegisters(driver);
    if (status != SERVO_OK) {
        return status;
    }

    // Запустити калібрування (магніт має обертатися 60-2000 RPM)
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_ACCURACY_START);
    if (status != SERVO_OK) {
        return status;
    }

    // Зачекати завершення калібрування (~2 секунди)
    HWD_Timer_DelayMs(AEAT9922_CALIB_TIME_MS);

    // Перевірити статус калібрування
    uint8_t calib_status;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CALIB_STATUS,
                                    &calib_status);
    if (status != SERVO_OK) {
        return status;
    }

    // Біти [1:0]: 10 = Pass, 11 = Fail
    uint8_t calib_result = calib_status & 0x03;
    if (calib_result != AEAT9922_CALIB_ACC_PASS) {
        return SERVO_ERROR;  // Calibration failed
    }

    // Вийти з режиму калібрування
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_EXIT);

    return status;
}

Servo_Status_t AEAT9922_CalibrateZero(AEAT9922_Driver_t* driver)
{
    Servo_Status_t status;

    // Розблокувати регістри
    status = AEAT9922_UnlockRegisters(driver);
    if (status != SERVO_OK) {
        return status;
    }

    // Запустити zero reset (вал має бути нерухомий в нульовій позиції)
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_ZERO_START);
    if (status != SERVO_OK) {
        return status;
    }

    // Зачекати завершення
    HWD_Timer_DelayMs(100);

    // Перевірити статус
    uint8_t calib_status;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CALIB_STATUS,
                                    &calib_status);
    if (status != SERVO_OK) {
        return status;
    }

    // Біти [3:2]: 10 = Pass, 11 = Fail
    uint8_t calib_result = (calib_status >> 2) & 0x03;
    if (calib_result != AEAT9922_CALIB_ACC_PASS) {
        return SERVO_ERROR;
    }

    // Вийти
    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CALIBRATE,
                                     AEAT9922_CALIB_EXIT);

    return status;
}

Servo_Status_t AEAT9922_UpdateIncrementalCount(AEAT9922_Driver_t* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    // Перевірити чи увімкнений режим ABI з апаратним таймером
    if (!(driver->config.enabled_modes & AEAT9922_MODE_ABI) ||
        !driver->config.abi.enable_incremental) {
        return SERVO_INVALID;
    }

    if (driver->config.abi.encoder_read == NULL) {
        return SERVO_INVALID;
    }

    // Зчитати поточний лічильник через callback
    int32_t current_count = driver->config.abi.encoder_read(driver->config.abi.encoder_ctx);

    // Обчислити різницю
    int32_t delta = current_count - driver->last_incremental_count;

    // Оновити загальний лічильник
    driver->incremental_count += delta;
    driver->last_incremental_count = current_count;

    return SERVO_OK;
}

void AEAT9922_IndexPulseCallback(AEAT9922_Driver_t* driver)
{
    if (driver != NULL) {
        // Підрахувати повний оберт (можна використовувати для верифікації multi-turn)
        driver->interface.data.revolution_count++;
    }
}

#endif /* USE_SENSOR_AEAT9922 */
