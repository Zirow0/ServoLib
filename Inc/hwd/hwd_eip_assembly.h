/*******************************************************************************
 * @file    hwd_eip_assembly.h
 * @brief   EtherNet/IP Assembly Object Management
 * @details Динамічне пакування/розпакування CIP Assembly Objects
 *          Підтримка Structure Discovery через Explicit Messaging
 *
 * @note    Compile-time enable: #define USE_HWD_EIP
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#ifndef HWD_EIP_ASSEMBLY_H
#define HWD_EIP_ASSEMBLY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config_lib.h"
#include "hwd_eip.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef USE_HWD_EIP

/*******************************************************************************
 * DEFINES
 ******************************************************************************/

/** Maximum number of fields у телеметрії */
#define HWD_EIP_MAX_TELEMETRY_FIELDS    6

/** Telemetry field types для descriptor */
typedef enum {
    HWD_EIP_FIELD_TYPE_UINT16 = 0,   /**< uint16_t */
    HWD_EIP_FIELD_TYPE_INT32  = 1,   /**< int32_t */
    HWD_EIP_FIELD_TYPE_FLOAT  = 2,   /**< float */
    HWD_EIP_FIELD_TYPE_UINT32 = 3    /**< uint32_t */
} HWD_EIP_FieldType_t;

/*******************************************************************************
 * TYPES
 ******************************************************************************/

/**
 * @brief Sensor field configuration
 */
typedef struct {
    bool enabled;           /**< Field enabled flag */
    uint16_t offset;        /**< Offset у assembly buffer (bytes) */
    uint8_t size;           /**< Field size (bytes) */
    HWD_EIP_FieldType_t type; /**< Data type */
    const char* name;       /**< Field name (для descriptor) */
} HWD_EIP_Field_t;

/**
 * @brief Assembly Layout (динамічна структура телеметрії)
 */
typedef struct {
    HWD_EIP_Field_t header;         /**< Header field (завжди є) */
    HWD_EIP_Field_t position;       /**< Position field */
    HWD_EIP_Field_t velocity;       /**< Velocity field */
    HWD_EIP_Field_t current;        /**< Current field */
    HWD_EIP_Field_t voltage;        /**< Voltage field */
    HWD_EIP_Field_t temperature;    /**< Temperature field */

    uint16_t total_size;            /**< Total assembly size (bytes) */
    uint8_t field_count;            /**< Number of enabled fields */
} HWD_EIP_AssemblyLayout_t;

/**
 * @brief Assembly Descriptor для Explicit Messaging
 * @note Відправляється Controller при GET_ATTRIBUTE_SINGLE
 */
typedef struct {
    uint16_t total_size;            /**< Total size у bytes */
    uint8_t  field_count;           /**< Number of fields */
    uint8_t  reserved;              /**< Alignment */

    struct {
        uint16_t offset;            /**< Field offset */
        uint8_t  size;              /**< Field size */
        uint8_t  type;              /**< Field type */
        char     name[16];          /**< Field name */
    } fields[HWD_EIP_MAX_TELEMETRY_FIELDS];
} HWD_EIP_AssemblyDescriptor_t;

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * @brief Побудова динамічного assembly layout
 *
 * @param layout    Вказівник на layout структуру
 * @param sensors   Вказівник на sensor configuration
 *
 * @note Викликати один раз при ініціалізації
 */
void HWD_EIP_Assembly_BuildLayout(
    HWD_EIP_AssemblyLayout_t* layout,
    const HWD_EIP_SensorConfig_t* sensors
);

/**
 * @brief Розпакування Command Assembly (Output)
 *
 * @param buffer    Assembly buffer (8 bytes)
 * @param command   Вказівник для запису команди
 * @return Servo_Status_t
 *         - SERVO_OK: успішне розпакування
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_Assembly_UnpackCommand(
    const uint8_t* buffer,
    HWD_EIP_Command_t* command
);

/**
 * @brief Пакування Command Assembly (для тестів)
 *
 * @param command   Вказівник на команду
 * @param buffer    Assembly buffer (8 bytes)
 * @return Servo_Status_t
 *         - SERVO_OK: успішне пакування
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_Assembly_PackCommand(
    const HWD_EIP_Command_t* command,
    uint8_t* buffer
);

/**
 * @brief Пакування Telemetry Assembly (Input) - динамічно
 *
 * @param buffer        Assembly buffer (max 32 bytes)
 * @param layout        Вказівник на assembly layout
 * @param header        Вказівник на header (завжди потрібен)
 * @param position      Вказівник на position (NULL якщо disabled)
 * @param velocity      Вказівник на velocity (NULL якщо disabled)
 * @param current       Вказівник на current (NULL якщо disabled)
 * @param voltage       Вказівник на voltage (NULL якщо disabled)
 * @param temperature   Вказівник на temperature (NULL якщо disabled)
 * @return Servo_Status_t
 *         - SERVO_OK: успішне пакування
 *         - SERVO_ERROR_NULL_PTR: null pointer для required fields
 *         - SERVO_INVALID: layout/field mismatch
 */
Servo_Status_t HWD_EIP_Assembly_PackTelemetry(
    uint8_t* buffer,
    const HWD_EIP_AssemblyLayout_t* layout,
    const HWD_EIP_Telemetry_Header_t* header,
    const int32_t* position,
    const float* velocity,
    const float* current,
    const float* voltage,
    const float* temperature
);

/**
 * @brief Розпакування Telemetry Assembly (для тестів)
 *
 * @param buffer        Assembly buffer
 * @param layout        Вказівник на assembly layout
 * @param header        Вказівник для запису header
 * @param position      Вказівник для запису position (NULL якщо не потрібно)
 * @param velocity      Вказівник для запису velocity (NULL якщо не потрібно)
 * @param current       Вказівник для запису current (NULL якщо не потрібно)
 * @param voltage       Вказівник для запису voltage (NULL якщо не потрібно)
 * @param temperature   Вказівник для запису temperature (NULL якщо не потрібно)
 * @return Servo_Status_t
 *         - SERVO_OK: успішне розпакування
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_Assembly_UnpackTelemetry(
    const uint8_t* buffer,
    const HWD_EIP_AssemblyLayout_t* layout,
    HWD_EIP_Telemetry_Header_t* header,
    int32_t* position,
    float* velocity,
    float* current,
    float* voltage,
    float* temperature
);

/**
 * @brief Формування Assembly Descriptor для Explicit Messaging
 *
 * @param layout      Вказівник на assembly layout
 * @param descriptor  Вказівник для запису descriptor
 * @return Servo_Status_t
 *         - SERVO_OK: descriptor сформовано
 *         - SERVO_ERROR_NULL_PTR: null pointer
 */
Servo_Status_t HWD_EIP_Assembly_BuildDescriptor(
    const HWD_EIP_AssemblyLayout_t* layout,
    HWD_EIP_AssemblyDescriptor_t* descriptor
);

/**
 * @brief Отримати розмір assembly
 *
 * @param layout    Вказівник на assembly layout
 * @return uint16_t Розмір у bytes
 */
uint16_t HWD_EIP_Assembly_GetSize(const HWD_EIP_AssemblyLayout_t* layout);

/**
 * @brief Отримати кількість полів
 *
 * @param layout    Вказівник на assembly layout
 * @return uint8_t  Кількість enabled fields
 */
uint8_t HWD_EIP_Assembly_GetFieldCount(const HWD_EIP_AssemblyLayout_t* layout);

/**
 * @brief Перевірка чи поле enabled
 *
 * @param field     Вказівник на field
 * @return bool     true якщо enabled
 */
static inline bool HWD_EIP_Assembly_IsFieldEnabled(const HWD_EIP_Field_t* field) {
    return field->enabled;
}

#endif /* USE_HWD_EIP */

#ifdef __cplusplus
}
#endif

#endif /* HWD_EIP_ASSEMBLY_H */
