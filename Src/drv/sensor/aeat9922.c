/**
 * @file aeat9922.c
 * @brief Реалізація драйвера магнітного енкодера AEAT-9922
 * @author ServoCore Team
 * @date 2025
 */

/* Includes ------------------------------------------------------------------*/
#include "board_config.h"

#ifdef USE_SENSOR_AEAT9922

/* Auto-enable dependencies */
#ifndef USE_SENSOR_POSITION
	#define USE_SENSOR_POSITION
#endif

#include "drv/sensor/aeat9922.h"
#include "hwd/hwd_timer.h"
#include "util/math.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/

// Макроси для SPI команд (відповідно до протоколу AEAT-9922)
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

static float aeat9922_position_to_degrees(uint32_t position,
                                          AEAT9922_Abs_Resolution_t res);

/**
 * @brief Мікросекундна затримка (приблизна, для SPI timing)
 * @param us Мікросекунди
 */
static inline void delay_us(uint32_t us)
{
    // Приблизно 25 циклів на мікросекунду при 100 MHz CPU
    volatile uint32_t cycles = us * 25;
    while (cycles--);
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

    // Налаштування Sensor Interface
    driver->interface.driver_data = driver;
    driver->interface.init = NULL;  // Використовуємо AEAT9922_Init напряму
    driver->interface.deinit = NULL;  // Використовуємо AEAT9922_DeInit напряму
    driver->interface.read_angle = AEAT9922_ReadAngle;
    driver->interface.read_velocity = AEAT9922_GetVelocity;
    driver->interface.calibrate = NULL;
    driver->interface.self_test = NULL;
    driver->interface.get_state = NULL;
    driver->interface.get_stats = NULL;

    return SERVO_OK;
}

Servo_Status_t AEAT9922_Init(void* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)driver;
    Servo_Status_t status;

    // 1. Встановити MSEL = HIGH для SPI4 режиму
    HAL_GPIO_WritePin((GPIO_TypeDef*)enc->config.msel_port,
                      enc->config.msel_pin, GPIO_PIN_SET);

    // 2. Ініціалізація SPI
    status = HWD_SPI_Init(&enc->spi_handle, &enc->config.spi_config);
    if (status != SERVO_OK) {
        return status;
    }

    // 3. Зачекати Power-Up час (10 ms)
    HAL_Delay(AEAT9922_POWERUP_TIME_MS);

    // 4. Перевірити статус енкодера
    status = AEAT9922_ReadStatus(enc);
    if (status != SERVO_OK) {
        return status;
    }

    if (!enc->status.ready) {
        return SERVO_ERROR;
    }

    // 5. Розблокувати регістри
    status = AEAT9922_UnlockRegisters(enc);
    if (status != SERVO_OK) {
        return status;
    }

    // 6. Налаштувати абсолютну роздільність
    uint8_t config1;
    status = AEAT9922_ReadRegister(enc, AEAT9922_REG_CONFIG1, &config1);
    if (status != SERVO_OK) {
        return status;
    }

    config1 = (config1 & 0xF0) | (enc->config.abs_resolution & 0x0F);

    // Додати налаштування напрямку
    if (enc->config.direction_ccw) {
        config1 |= (1 << 4);  // Біт 4: Direction (CCW count up)
    } else {
        config1 &= ~(1 << 4);
    }

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_CONFIG1, config1);
    if (status != SERVO_OK) {
        return status;
    }

    // 7. Налаштувати інкрементальну роздільність
    uint16_t cpr = enc->config.incremental_cpr;
    if (cpr < 1) cpr = 1;
    if (cpr > 10000) cpr = 10000;

    uint8_t cpr_high = (cpr >> 8) & 0x3F;
    uint8_t cpr_low = cpr & 0xFF;

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_INC_RES_HIGH, cpr_high);
    if (status != SERVO_OK) {
        return status;
    }

    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_INC_RES_LOW, cpr_low);
    if (status != SERVO_OK) {
        return status;
    }

    // 8. Налаштувати PSEL для вибору інтерфейсу SPI4
    uint8_t config2;
    status = AEAT9922_ReadRegister(enc, AEAT9922_REG_CONFIG2, &config2);
    if (status != SERVO_OK) {
        return status;
    }

    config2 = (config2 & 0x9F) | ((enc->config.interface_mode & 0x03) << 5);
    status = AEAT9922_WriteRegister(enc, AEAT9922_REG_CONFIG2, config2);
    if (status != SERVO_OK) {
        return status;
    }

    // 9. Ініціалізувати інкрементальний лічильник (якщо використовується)
    if (enc->config.enable_incremental && enc->config.encoder_timer_handle != NULL) {
        TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)enc->config.encoder_timer_handle;
        HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
        enc->incremental_count = 0;
        enc->last_incremental_count = 0;
    }

    // 10. Ініціалізувати часові мітки
    enc->last_update_time = HAL_GetTick();
    enc->last_angle = 0.0f;

    return SERVO_OK;
}

Servo_Status_t AEAT9922_DeInit(void* driver)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)driver;

    // Зупинити інкрементальний таймер
    if (enc->config.enable_incremental && enc->config.encoder_timer_handle != NULL) {
        TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)enc->config.encoder_timer_handle;
        HAL_TIM_Encoder_Stop(htim, TIM_CHANNEL_ALL);
    }

    // Деініціалізувати SPI
    return HWD_SPI_DeInit(&enc->spi_handle);
}

Servo_Status_t AEAT9922_ReadAngle(Sensor_Interface_t* iface, float* angle)
{
    if (iface == NULL || angle == NULL || iface->driver_data == NULL) {
        return SERVO_INVALID;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)iface->driver_data;
    Servo_Status_t status;
    uint8_t tx_data[3];
    uint8_t rx_data[3] = {0};

    // Відправити команду читання позиції (register 0x3F)
    tx_data[0] = AEAT9922_CMD_READ(AEAT9922_REG_POSITION);  // 0x40
    tx_data[1] = AEAT9922_REG_POSITION;                     // 0x3F
    tx_data[2] = 0x00;                                       // Dummy

    HWD_SPI_CS_Low(&enc->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    status = HWD_SPI_TransmitReceive(&enc->spi_handle, tx_data, rx_data, 3);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&enc->spi_handle);

    if (status != SERVO_OK) {
        enc->error_count++;
        return status;
    }

    // Розпакувати дані відповідно до протоколу AEAT-9922
    // Формат 24-біт відповіді: [4_reserved][W][E][18-bit_position]
    // rx_data[0]: біти [23:16]
    // rx_data[1]: біти [15:8]
    // rx_data[2]: біти [7:0]

    uint32_t position_24bit = ((uint32_t)rx_data[0] << 16) |
                               ((uint32_t)rx_data[1] << 8) |
                               rx_data[2];

    // Витягнути біти статусу
    // uint8_t warning = (position_24bit >> 19) & 0x01;  // W bit
    // uint8_t error = (position_24bit >> 18) & 0x01;    // E bit

    // Витягнути позицію (біти [17:0])
    uint32_t position_18bit = position_24bit & 0x3FFFF;

    // Застосувати маску відповідно до налаштованої роздільності
    uint32_t resolution_bits = 18 - (uint8_t)enc->config.abs_resolution;
    uint32_t position_mask = (1 << resolution_bits) - 1;
    enc->raw_position = position_18bit & position_mask;

    // Конвертувати в градуси
    enc->angle_degrees = aeat9922_position_to_degrees(enc->raw_position,
                                                       enc->config.abs_resolution);
    *angle = enc->angle_degrees;

    // Оновити швидкість
    uint32_t current_time = HAL_GetTick();
    uint32_t dt = current_time - enc->last_update_time;

    if (dt > 0) {
        float angle_diff = enc->angle_degrees - enc->last_angle;

        // Обробка переходу через 0/360
        if (angle_diff > 180.0f) {
            angle_diff -= 360.0f;
        } else if (angle_diff < -180.0f) {
            angle_diff += 360.0f;
        }

        enc->velocity = (angle_diff * 1000.0f) / (float)dt;  // град/с
        enc->last_angle = enc->angle_degrees;
        enc->last_update_time = current_time;
    }

    return SERVO_OK;
}

Servo_Status_t AEAT9922_GetVelocity(Sensor_Interface_t* iface, float* velocity)
{
    if (iface == NULL || velocity == NULL || iface->driver_data == NULL) {
        return SERVO_INVALID;
    }

    AEAT9922_Driver_t* enc = (AEAT9922_Driver_t*)iface->driver_data;
    *velocity = enc->velocity;

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

    uint8_t tx_data[2];
    uint8_t rx_data[2] = {0};
    Servo_Status_t status;

    // Крок 1: Відправити команду читання (RW=1, адреса)
    tx_data[0] = AEAT9922_CMD_READ(address);  // 0x40 (RW=1)
    tx_data[1] = address;                     // Адреса регістру

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, 2);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status != SERVO_OK) {
        return status;
    }

    delay_us(2);  // t_CSR >= 350ns (між транзакціями)

    // Крок 2: Зчитати дані в наступній транзакції
    tx_data[0] = 0x00;  // Dummy byte
    tx_data[1] = 0x00;  // Dummy byte

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);

    status = HWD_SPI_TransmitReceive(&driver->spi_handle, tx_data, rx_data, 2);

    delay_us(1);
    HWD_SPI_CS_High(&driver->spi_handle);

    if (status == SERVO_OK) {
        *value = rx_data[0];  // Дані в першому байті відповіді
    }

    return status;
}

Servo_Status_t AEAT9922_WriteRegister(AEAT9922_Driver_t* driver,
                                       uint8_t address, uint8_t value)
{
    if (driver == NULL) {
        return SERVO_INVALID;
    }

    uint8_t tx_data[3];
    uint8_t rx_data[3] = {0};  // Dummy receive buffer

    // Команда запису: RW=0 (bit 6 = 0), адреса, дані
    tx_data[0] = AEAT9922_CMD_WRITE(address);  // 0x00 (RW=0)
    tx_data[1] = address;                       // Адреса регістру
    tx_data[2] = value;                         // Дані для запису

    HWD_SPI_CS_Low(&driver->spi_handle);
    delay_us(1);  // t_CSn >= 350ns

    Servo_Status_t status = HWD_SPI_TransmitReceive(&driver->spi_handle,
                                                     tx_data, rx_data, 3);

    delay_us(1);  // t_CSf >= 50ns
    HWD_SPI_CS_High(&driver->spi_handle);

    // Час на обробку запису (мінімум 1 ms)
    HAL_Delay(1);

    return status;
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
    if (driver == NULL || !driver->config.enable_incremental) {
        return SERVO_INVALID;
    }

    TIM_HandleTypeDef* htim = (TIM_HandleTypeDef*)driver->config.encoder_timer_handle;
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
        // Підрахувати повний оберт
        driver->revolution_count++;
    }
}

Sensor_Interface_t* AEAT9922_GetInterface(AEAT9922_Driver_t* driver)
{
    if (driver == NULL) {
        return NULL;
    }

    return &driver->interface;
}

/* Private functions ---------------------------------------------------------*/

static float aeat9922_position_to_degrees(uint32_t position,
                                          AEAT9922_Abs_Resolution_t res)
{
    // Обчислити максимальне значення для даної роздільної здатності
    uint32_t max_count = 1 << (18 - (uint8_t)res);

    // Конвертувати в градуси (0-360)
    float degrees = ((float)position * 360.0f) / (float)max_count;

    // Обмежити діапазон 0-360
    if (degrees >= 360.0f) {
        degrees -= 360.0f;
    }
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }

    return degrees;
}

#endif /* USE_SENSOR_AEAT9922 */
