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
#include "util/checksum.h"
#include "stm32f4xx_hal.h"
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
    GPIO_PinState msel_state = GPIO_PIN_RESET;
    if (modes & (AEAT9922_MODE_SPI4 | AEAT9922_MODE_PWM | AEAT9922_MODE_UVW)) {
        msel_state = GPIO_PIN_SET;
    }
    HAL_GPIO_WritePin((GPIO_TypeDef*)driver->config.spi_config.msel_port,
                      driver->config.spi_config.msel_pin, msel_state);

    // 2. Ініціалізація SPI (якщо використовується будь-який SPI режим)
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        status = HWD_SPI_Init(&driver->spi_handle, &driver->config.spi_config.spi_config);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // 3. Зачекати Power-Up час (10 ms)
    HAL_Delay(AEAT9922_POWERUP_TIME_MS);

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

    // 5. Розблокувати регістри для конфігурації
    if (modes & (AEAT9922_MODE_SPI3 | AEAT9922_MODE_SPI4)) {
        status = AEAT9922_UnlockRegisters(driver);
        if (status != SERVO_OK) {
            return status;
        }
    }

    // 6. Налаштувати абсолютну роздільність (CONFIG1)
    uint8_t config1;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CONFIG1, &config1);
    if (status != SERVO_OK) {
        return status;
    }

    // Встановити роздільність (біти [3:0])
    config1 = (config1 & 0xF0) | (driver->config.general.abs_resolution & 0x0F);

    // Встановити напрямок (біт 4)
    if (driver->config.general.direction_ccw) {
        config1 |= (1 << 4);  // CCW count up
    } else {
        config1 &= ~(1 << 4); // CW count up
    }

    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CONFIG1, config1);
    if (status != SERVO_OK) {
        return status;
    }

    // 7. Налаштувати інкрементальну роздільність (якщо режим ABI увімкнений)
    if (modes & AEAT9922_MODE_ABI) {
        uint16_t cpr = driver->config.abi.incremental_cpr;
        if (cpr < AEAT9922_INC_CPR_MIN) cpr = AEAT9922_INC_CPR_MIN;
        if (cpr > AEAT9922_INC_CPR_MAX) cpr = AEAT9922_INC_CPR_MAX;

        uint8_t cpr_high = (cpr >> 8) & 0x3F;
        uint8_t cpr_low = cpr & 0xFF;

        status = AEAT9922_WriteRegister(driver, AEAT9922_REG_INC_RES_HIGH, cpr_high);
        if (status != SERVO_OK) {
            return status;
        }

        status = AEAT9922_WriteRegister(driver, AEAT9922_REG_INC_RES_LOW, cpr_low);
        if (status != SERVO_OK) {
            return status;
        }

        // Налаштувати Index pulse width та state (CONFIG0)
        // TODO: Implement CONFIG0 write for index_width and index_state
    }

    // 8. Налаштувати PSEL для вибору варіанту протоколу (CONFIG2)
    uint8_t config2;
    status = AEAT9922_ReadRegister(driver, AEAT9922_REG_CONFIG2, &config2);
    if (status != SERVO_OK) {
        return status;
    }

    // PSEL[1:0] в бітах [6:5]
    config2 = (config2 & 0x9F) | ((driver->config.spi_config.protocol_variant & 0x03) << 5);

    status = AEAT9922_WriteRegister(driver, AEAT9922_REG_CONFIG2, config2);
    if (status != SERVO_OK) {
        return status;
    }

    // 9. Ініціалізувати апаратний таймер для ABI (якщо використовується)
    if ((modes & AEAT9922_MODE_ABI) && driver->config.abi.enable_incremental) {
        if (driver->config.abi.encoder_timer_handle != NULL) {
            TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)driver->config.abi.encoder_timer_handle;
            HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
            driver->incremental_count = 0;
            driver->last_incremental_count = 0;
        }
    }

    // 10. Виконати auto zero калібрування (якщо увімкнено)
    if (driver->config.general.auto_zero_on_init) {
        status = AEAT9922_CalibrateZero(driver);
        if (status != SERVO_OK) {
            return status;
        }
    }

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
        if (driver->config.abi.encoder_timer_handle != NULL) {
            TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)driver->config.abi.encoder_timer_handle;
            HAL_TIM_Encoder_Stop(htim, TIM_CHANNEL_ALL);
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
 * SPI4-B Protocol (24-bit з CRC):
 * TX: [CMD | ADDR | DUMMY]
 * RX: [Reserved(4)|W|E|Data[17:10]] [Data[9:2]] [CRC-8]
 */
static Servo_Status_t AEAT9922_HW_ReadRaw(void* driver_data, Position_Raw_Data_t* raw)
{
    AEAT9922_Driver_t* driver = (AEAT9922_Driver_t*)driver_data;
    Servo_Status_t status;
    uint8_t tx_data[3];
    uint8_t rx_data[3] = {0};

    // Відправити команду читання позиції (register 0x3F)
    tx_data[0] = AEAT9922_CMD_READ(AEAT9922_REG_POSITION);  // 0x40
    tx_data[1] = AEAT9922_REG_POSITION;                     // 0x3F

    // Розрахувати CRC для TX пакету (якщо SPI4-B)
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        tx_data[2] = Checksum_CRC8(tx_data, 2, CRC8_POLY_DEFAULT);
    } else {
        tx_data[2] = 0x00;  // Dummy для інших режимів
    }

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, 3);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        driver->error_count++;
        raw->valid = false;
        return status;
    }

    /* ========================================================================
     * SPI4-B РОЗПАКОВКА (24-bit з CRC-8)
     * ========================================================================
     * Формат відповіді:
     * Байт 0 [23:16]: Reserved(4) | W | E | Data[17:16]
     * Байт 1 [15:8]:  Data[15:8]
     * Байт 2 [7:0]:   CRC-8
     *
     * Біти:
     * [23:22] - Reserved (завжди 0)
     * [22]    - W (Warning): магніт не в оптимальній позиції
     * [21]    - E (Error): критична помилка комунікації
     * [20:5]  - Position data (16 біт для 18-bit роздільності)
     * [7:0]   - CRC-8 контрольна сума
     */

    // Перевірка CRC-8 (якщо SPI4-B режим активний)
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        uint8_t calculated_crc = Checksum_CRC8(rx_data, 2, CRC8_POLY_DEFAULT);
        uint8_t received_crc = rx_data[2];

        if (calculated_crc != received_crc) {
            // CRC помилка
            driver->error_count++;
            raw->valid = false;
            return SERVO_ERROR;
        }
    }

    // Витягнути прапорці W та E
    bool warning_flag = (rx_data[0] & (1 << 6)) != 0;  // W bit
    bool error_flag   = (rx_data[0] & (1 << 5)) != 0;  // E bit

    // Обробити помилки
    if (error_flag) {
        driver->error_count++;
        raw->valid = false;
        return SERVO_ERROR;
    }

    if (warning_flag) {
        // Попередження: магніт не в оптимальній позиції
        // Можна логувати, але продовжуємо роботу
    }

    // Витягнути дані позиції (біти [20:5] = 16 біт)
    // Для 18-bit роздільності: rx_data[0] містить [Data17:Data16], rx_data[1] містить [Data15:Data8]
    uint32_t position_18bit = ((uint32_t)(rx_data[0] & 0x03) << 16) |  // Біти [17:16]
                              ((uint32_t)rx_data[1] << 8);               // Біти [15:8]
    // Біти [7:0] відсутні в SPI4-B для позиції (CRC займає байт 2)

    // Застосувати маску відповідно до налаштованої роздільності
    uint32_t resolution_bits = 18 - (uint8_t)driver->config.general.abs_resolution;
    uint32_t position_mask = (1U << resolution_bits) - 1;
    uint32_t raw_position = position_18bit & position_mask;

    // Заповнити структуру Position_Raw_Data_t
    raw->raw_position = raw_position;
    raw->timestamp_us = HWD_Timer_GetMicros();
    raw->has_velocity = false;  // AEAT-9922 НЕ надає готову velocity
    raw->raw_velocity = 0.0f;
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
    driver->interface.resolution_bits = 18 - (uint8_t)config->general.abs_resolution;
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
    uint8_t packet_size = 2;  // За замовчуванням SPI3
    if (driver->config.spi_config.protocol_variant == AEAT9922_PSEL_SPI4_24BIT) {
        packet_size = 3;  // SPI4-B з CRC
    }

    uint8_t tx_data[3] = {0};
    uint8_t rx_data[3] = {0};

    // Крок 1: Відправити команду читання (RW=1, адреса)
    tx_data[0] = AEAT9922_CMD_READ(address);  // 0x40 (RW=1)
    tx_data[1] = address;                     // Адреса регістру

    // Розрахувати CRC для SPI4-B
    if (packet_size == 3) {
        tx_data[2] = Checksum_CRC8(tx_data, 2, CRC8_POLY_DEFAULT);
    } else {
        tx_data[2] = 0x00;  // Не використовується для SPI3
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

    // Перевірка CRC для SPI4-B
    if (packet_size == 3) {
        uint8_t calculated_crc = Checksum_CRC8(rx_data, 2, CRC8_POLY_DEFAULT);
        uint8_t received_crc = rx_data[2];

        if (calculated_crc != received_crc) {
            // CRC помилка
            return SERVO_ERROR;
        }
    }

    // Витягнути дані (перший байт містить регістр)
    *value = rx_data[1];

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
    HAL_Delay(1);

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
    HAL_Delay(AEAT9922_EEPROM_WRITE_TIME_MS);

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
    HAL_Delay(AEAT9922_CALIB_TIME_MS);

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
    HAL_Delay(100);

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

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)driver->config.abi.encoder_timer_handle;
    if (htim == NULL) {
        return SERVO_INVALID;
    }

    // Зчитати поточний лічильник з таймера
    int32_t current_count = (int32_t)__HAL_TIM_GET_COUNTER(htim);

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
