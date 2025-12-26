/*******************************************************************************
 * @file    hwd_eip_assembly.c
 * @brief   EtherNet/IP Assembly Object Management Implementation
 * @details Динамічне пакування/розпакування CIP Assembly Objects
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#include "hwd/hwd_eip_assembly.h"

#ifdef USE_HWD_EIP

#include <string.h>

/*******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * @brief Ініціалізація field з параметрами
 */
static void assembly_init_field(
    HWD_EIP_Field_t* field,
    bool enabled,
    uint16_t offset,
    uint8_t size,
    HWD_EIP_FieldType_t type,
    const char* name
) {
    field->enabled = enabled;
    field->offset = offset;
    field->size = size;
    field->type = type;
    field->name = name;
}

/**
 * @brief Копіювання bytes з alignment
 */
static inline void copy_bytes(uint8_t* dest, const void* src, size_t size) {
    memcpy(dest, src, size);
}

/**
 * @brief Читання bytes з alignment
 */
static inline void read_bytes(void* dest, const uint8_t* src, size_t size) {
    memcpy(dest, src, size);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

void HWD_EIP_Assembly_BuildLayout(
    HWD_EIP_AssemblyLayout_t* layout,
    const HWD_EIP_SensorConfig_t* sensors
) {
    if (layout == NULL || sensors == NULL) {
        return;
    }

    memset(layout, 0, sizeof(HWD_EIP_AssemblyLayout_t));

    uint16_t current_offset = 0;
    uint8_t field_count = 0;

    /* Header (завжди присутній) - 8 bytes */
    assembly_init_field(&layout->header, true, current_offset, 8,
                       HWD_EIP_FIELD_TYPE_UINT16, "header");
    current_offset += 8;
    field_count++;

    /* Position sensor - 4 bytes (int32_t) */
    if (sensors->position_enabled) {
        assembly_init_field(&layout->position, true, current_offset, 4,
                           HWD_EIP_FIELD_TYPE_INT32, "position_counts");
        current_offset += 4;
        field_count++;
    } else {
        assembly_init_field(&layout->position, false, 0, 0,
                           HWD_EIP_FIELD_TYPE_INT32, "position_counts");
    }

    /* Velocity - 4 bytes (float) */
    if (sensors->velocity_enabled) {
        assembly_init_field(&layout->velocity, true, current_offset, 4,
                           HWD_EIP_FIELD_TYPE_FLOAT, "velocity_rad_s");
        current_offset += 4;
        field_count++;
    } else {
        assembly_init_field(&layout->velocity, false, 0, 0,
                           HWD_EIP_FIELD_TYPE_FLOAT, "velocity_rad_s");
    }

    /* Current sensor - 4 bytes (float) */
    if (sensors->current_enabled) {
        assembly_init_field(&layout->current, true, current_offset, 4,
                           HWD_EIP_FIELD_TYPE_FLOAT, "motor_current_A");
        current_offset += 4;
        field_count++;
    } else {
        assembly_init_field(&layout->current, false, 0, 0,
                           HWD_EIP_FIELD_TYPE_FLOAT, "motor_current_A");
    }

    /* Voltage sensor - 4 bytes (float) */
    if (sensors->voltage_enabled) {
        assembly_init_field(&layout->voltage, true, current_offset, 4,
                           HWD_EIP_FIELD_TYPE_FLOAT, "motor_voltage_V");
        current_offset += 4;
        field_count++;
    } else {
        assembly_init_field(&layout->voltage, false, 0, 0,
                           HWD_EIP_FIELD_TYPE_FLOAT, "motor_voltage_V");
    }

    /* Temperature sensor - 4 bytes (float) */
    if (sensors->temperature_enabled) {
        assembly_init_field(&layout->temperature, true, current_offset, 4,
                           HWD_EIP_FIELD_TYPE_FLOAT, "winding_temp_C");
        current_offset += 4;
        field_count++;
    } else {
        assembly_init_field(&layout->temperature, false, 0, 0,
                           HWD_EIP_FIELD_TYPE_FLOAT, "winding_temp_C");
    }

    layout->total_size = current_offset;
    layout->field_count = field_count;
}

Servo_Status_t HWD_EIP_Assembly_UnpackCommand(
    const uint8_t* buffer,
    HWD_EIP_Command_t* command
) {
    if (buffer == NULL || command == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Розпакувати у порядку структури */
    uint16_t offset = 0;

    /* control_word - 2 bytes */
    read_bytes(&command->control_word, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    /* pwm_duty_percent - 4 bytes */
    read_bytes(&command->pwm_duty_percent, buffer + offset, sizeof(float));
    offset += sizeof(float);

    /* mode_of_operation - 1 byte */
    command->mode_of_operation = buffer[offset];
    offset += 1;

    /* brake_command - 1 byte */
    command->brake_command = buffer[offset];

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_Assembly_PackCommand(
    const HWD_EIP_Command_t* command,
    uint8_t* buffer
) {
    if (command == NULL || buffer == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Запакувати у порядку структури */
    uint16_t offset = 0;

    /* control_word - 2 bytes */
    copy_bytes(buffer + offset, &command->control_word, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    /* pwm_duty_percent - 4 bytes */
    copy_bytes(buffer + offset, &command->pwm_duty_percent, sizeof(float));
    offset += sizeof(float);

    /* mode_of_operation - 1 byte */
    buffer[offset] = command->mode_of_operation;
    offset += 1;

    /* brake_command - 1 byte */
    buffer[offset] = command->brake_command;

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_Assembly_PackTelemetry(
    uint8_t* buffer,
    const HWD_EIP_AssemblyLayout_t* layout,
    const HWD_EIP_Telemetry_Header_t* header,
    const int32_t* position,
    const float* velocity,
    const float* current,
    const float* voltage,
    const float* temperature
) {
    if (buffer == NULL || layout == NULL || header == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Clear buffer */
    memset(buffer, 0, layout->total_size);

    /* Pack header (завжди є) */
    uint16_t offset = 0;
    copy_bytes(buffer + offset, &header->status_word, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    copy_bytes(buffer + offset, &header->warning_code, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    copy_bytes(buffer + offset, &header->error_code, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    /* Pack опціональні поля */
    if (layout->position.enabled) {
        if (position == NULL) {
            return SERVO_ERROR_NULL_PTR;
        }
        copy_bytes(buffer + layout->position.offset, position, sizeof(int32_t));
    }

    if (layout->velocity.enabled) {
        if (velocity == NULL) {
            return SERVO_ERROR_NULL_PTR;
        }
        copy_bytes(buffer + layout->velocity.offset, velocity, sizeof(float));
    }

    if (layout->current.enabled) {
        if (current == NULL) {
            return SERVO_ERROR_NULL_PTR;
        }
        copy_bytes(buffer + layout->current.offset, current, sizeof(float));
    }

    if (layout->voltage.enabled) {
        if (voltage == NULL) {
            return SERVO_ERROR_NULL_PTR;
        }
        copy_bytes(buffer + layout->voltage.offset, voltage, sizeof(float));
    }

    if (layout->temperature.enabled) {
        if (temperature == NULL) {
            return SERVO_ERROR_NULL_PTR;
        }
        copy_bytes(buffer + layout->temperature.offset, temperature, sizeof(float));
    }

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_Assembly_UnpackTelemetry(
    const uint8_t* buffer,
    const HWD_EIP_AssemblyLayout_t* layout,
    HWD_EIP_Telemetry_Header_t* header,
    int32_t* position,
    float* velocity,
    float* current,
    float* voltage,
    float* temperature
) {
    if (buffer == NULL || layout == NULL || header == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Unpack header */
    uint16_t offset = 0;
    read_bytes(&header->status_word, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    read_bytes(&header->warning_code, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    read_bytes(&header->error_code, buffer + offset, sizeof(uint32_t));

    /* Unpack опціональні поля */
    if (layout->position.enabled && position != NULL) {
        read_bytes(position, buffer + layout->position.offset, sizeof(int32_t));
    }

    if (layout->velocity.enabled && velocity != NULL) {
        read_bytes(velocity, buffer + layout->velocity.offset, sizeof(float));
    }

    if (layout->current.enabled && current != NULL) {
        read_bytes(current, buffer + layout->current.offset, sizeof(float));
    }

    if (layout->voltage.enabled && voltage != NULL) {
        read_bytes(voltage, buffer + layout->voltage.offset, sizeof(float));
    }

    if (layout->temperature.enabled && temperature != NULL) {
        read_bytes(temperature, buffer + layout->temperature.offset, sizeof(float));
    }

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_Assembly_BuildDescriptor(
    const HWD_EIP_AssemblyLayout_t* layout,
    HWD_EIP_AssemblyDescriptor_t* descriptor
) {
    if (layout == NULL || descriptor == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    memset(descriptor, 0, sizeof(HWD_EIP_AssemblyDescriptor_t));

    descriptor->total_size = layout->total_size;
    descriptor->field_count = layout->field_count;

    uint8_t field_idx = 0;

    /* Додати header fields (status_word, warning_code, error_code як окремі) */
    /* Для простоти: header як один field */
    strncpy(descriptor->fields[field_idx].name, "status_word", 15);
    descriptor->fields[field_idx].offset = 0;
    descriptor->fields[field_idx].size = 2;
    descriptor->fields[field_idx].type = HWD_EIP_FIELD_TYPE_UINT16;
    field_idx++;

    strncpy(descriptor->fields[field_idx].name, "warning_code", 15);
    descriptor->fields[field_idx].offset = 2;
    descriptor->fields[field_idx].size = 2;
    descriptor->fields[field_idx].type = HWD_EIP_FIELD_TYPE_UINT16;
    field_idx++;

    strncpy(descriptor->fields[field_idx].name, "error_code", 15);
    descriptor->fields[field_idx].offset = 4;
    descriptor->fields[field_idx].size = 4;
    descriptor->fields[field_idx].type = HWD_EIP_FIELD_TYPE_UINT32;
    field_idx++;

    /* Додати опціональні поля */
    if (layout->position.enabled) {
        strncpy(descriptor->fields[field_idx].name, layout->position.name, 15);
        descriptor->fields[field_idx].offset = layout->position.offset;
        descriptor->fields[field_idx].size = layout->position.size;
        descriptor->fields[field_idx].type = layout->position.type;
        field_idx++;
    }

    if (layout->velocity.enabled) {
        strncpy(descriptor->fields[field_idx].name, layout->velocity.name, 15);
        descriptor->fields[field_idx].offset = layout->velocity.offset;
        descriptor->fields[field_idx].size = layout->velocity.size;
        descriptor->fields[field_idx].type = layout->velocity.type;
        field_idx++;
    }

    if (layout->current.enabled) {
        strncpy(descriptor->fields[field_idx].name, layout->current.name, 15);
        descriptor->fields[field_idx].offset = layout->current.offset;
        descriptor->fields[field_idx].size = layout->current.size;
        descriptor->fields[field_idx].type = layout->current.type;
        field_idx++;
    }

    if (layout->voltage.enabled) {
        strncpy(descriptor->fields[field_idx].name, layout->voltage.name, 15);
        descriptor->fields[field_idx].offset = layout->voltage.offset;
        descriptor->fields[field_idx].size = layout->voltage.size;
        descriptor->fields[field_idx].type = layout->voltage.type;
        field_idx++;
    }

    if (layout->temperature.enabled) {
        strncpy(descriptor->fields[field_idx].name, layout->temperature.name, 15);
        descriptor->fields[field_idx].offset = layout->temperature.offset;
        descriptor->fields[field_idx].size = layout->temperature.size;
        descriptor->fields[field_idx].type = layout->temperature.type;
        field_idx++;
    }

    /* Оновити field_count (header розбитий на 3) */
    descriptor->field_count = field_idx;

    return SERVO_OK;
}

uint16_t HWD_EIP_Assembly_GetSize(const HWD_EIP_AssemblyLayout_t* layout) {
    if (layout == NULL) {
        return 0;
    }
    return layout->total_size;
}

uint8_t HWD_EIP_Assembly_GetFieldCount(const HWD_EIP_AssemblyLayout_t* layout) {
    if (layout == NULL) {
        return 0;
    }
    return layout->field_count;
}

#endif /* USE_HWD_EIP */
