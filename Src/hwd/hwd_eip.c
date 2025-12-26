/*******************************************************************************
 * @file    hwd_eip.c
 * @brief   EtherNet/IP Hardware Driver Implementation
 * @details OpENer-based EtherNet/IP stack wrapper для ServoLib
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#include "hwd/hwd_eip.h"

#ifdef USE_HWD_EIP

#include "hwd/hwd_eip_assembly.h"
#include "hwd/hwd_eip_ds402.h"
#include <string.h>

/* OpENer includes */
#include "opener_api.h"
#include "appcontype.h"
#include "cipidentity.h"

/*******************************************************************************
 * PRIVATE VARIABLES
 ******************************************************************************/

/** Global assembly buffers для OpENer */
static uint8_t g_output_assembly_data[HWD_EIP_COMMAND_SIZE];     /* Commands від Controller */
static uint8_t g_input_assembly_data[HWD_EIP_TELEMETRY_MAX_SIZE]; /* Telemetry до Controller */

/** Assembly layout */
static HWD_EIP_AssemblyLayout_t g_assembly_layout;

/** Command/Telemetry buffers */
static HWD_EIP_Command_t g_last_command;
static bool g_command_received = false;

/** Watchdog */
static uint32_t g_last_command_time_ms = 0;

/*******************************************************************************
 * OPENER CALLBACKS (обов'язкові для OpENer)
 ******************************************************************************/

/**
 * @brief OpENer callback: Application initialization
 * @note Викликається OpENer при старті
 */
EipStatus ApplicationInitialization(void) {
    /* Create Assembly Objects */

    /* Output Assembly (Commands: Controller → Device) */
    CreateAssemblyObject(HWD_EIP_OUTPUT_ASSEMBLY_ID,
                        g_output_assembly_data,
                        HWD_EIP_COMMAND_SIZE);

    /* Input Assembly (Telemetry: Device → Controller) */
    CreateAssemblyObject(HWD_EIP_INPUT_ASSEMBLY_ID,
                        g_input_assembly_data,
                        g_assembly_layout.total_size);

    /* Config Assembly (unused) */
    CreateAssemblyObject(HWD_EIP_CONFIG_ASSEMBLY_ID, NULL, 0);

    /* Configure Connection Point */
    ConfigureExclusiveOwnerConnectionPoint(0,
                                          HWD_EIP_OUTPUT_ASSEMBLY_ID,
                                          HWD_EIP_INPUT_ASSEMBLY_ID,
                                          HWD_EIP_CONFIG_ASSEMBLY_ID);

    return kEipStatusOk;
}

/**
 * @brief OpENer callback: After Assembly data received
 * @note Викликається OpENer коли отримано дані від Controller
 */
EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
    if (instance->instance_number == HWD_EIP_OUTPUT_ASSEMBLY_ID) {
        /* Розпакувати команду */
        if (HWD_EIP_Assembly_UnpackCommand(g_output_assembly_data, &g_last_command) == SERVO_OK) {
            g_command_received = true;
            g_last_command_time_ms = HWD_Timer_GetMillis();
        }
    }

    return kEipStatusOk;
}

/**
 * @brief OpENer callback: Before Assembly data send
 * @note Викликається OpENer перед відправкою телеметрії
 */
EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
    (void)instance; /* Telemetry вже запаковано у HWD_EIP_SetTelemetry() */
    return true; /* Data is new */
}

/**
 * @brief OpENer callback: Handle application
 * @note Викликається OpENer у main loop
 */
void HandleApplication(void) {
    /* Nothing to do - вся логіка у HWD_EIP_Update() */
}

/**
 * @brief OpENer callback: Check I/O connection event
 */
void CheckIoConnectionEvent(unsigned int output_assembly_id,
                           unsigned int input_assembly_id,
                           IoConnectionEvent io_connection_event) {
    (void)output_assembly_id;
    (void)input_assembly_id;
    (void)io_connection_event;

    /* TODO: Handle connection events (connect/disconnect) */
}

/**
 * @brief OpENer callback: Reset device
 */
EipStatus ResetDevice(void) {
    /* TODO: Implement reset logic */
    return kEipStatusOk;
}

/**
 * @brief OpENer callback: Reset to initial configuration
 */
EipStatus ResetDeviceToInitialConfiguration(void) {
    ResetDevice();
    return kEipStatusOk;
}

/**
 * @brief OpENer callback: Memory allocation (якщо потрібно)
 */
void* CipCalloc(size_t number_of_elements, size_t size_of_element) {
    /* ServoLib використовує static allocation, тому повертаємо NULL */
    (void)number_of_elements;
    (void)size_of_element;
    return NULL;
}

/**
 * @brief OpENer callback: Memory free (якщо потрібно)
 */
void CipFree(void* data) {
    /* ServoLib не використовує dynamic allocation */
    (void)data;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

Servo_Status_t HWD_EIP_Init(HWD_EIP_Handle_t* handle, const HWD_EIP_Config_t* config) {
    if (handle == NULL || config == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Copy configuration */
    memcpy(&handle->config, config, sizeof(HWD_EIP_Config_t));

    /* Build assembly layout на основі sensor configuration */
    HWD_EIP_Assembly_BuildLayout(&g_assembly_layout, &config->sensors);
    handle->telemetry_size = g_assembly_layout.total_size;

    /* Initialize OpENer */
    if (CipStackInit(config->identity.serial_number) != kEipStatusOk) {
        return SERVO_ERROR;
    }

    /* Set device identity */
    SetDeviceVendorId(config->identity.vendor_id);
    SetDeviceType(config->identity.device_type);
    SetDeviceProductCode(config->identity.product_code);
    SetDeviceSerialNumber(config->identity.serial_number);
    SetDeviceRevision(config->identity.revision_major, config->identity.revision_minor);

    /* Initialize network handler (platform-specific) */
    if (NetworkHandlerInitialize() != 0) {
        return SERVO_ERROR;
    }

    /* Reset state */
    handle->initialized = true;
    handle->connection_active = false;
    handle->last_command_time_ms = 0;
    handle->timeout_count = 0;
    g_command_received = false;

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_Update(HWD_EIP_Handle_t* handle) {
    if (handle == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!handle->initialized) {
        return SERVO_NOT_INIT;
    }

    /* Process OpENer network events */
    if (NetworkHandlerProcessOnce() != kEipStatusOk) {
        return SERVO_ERROR;
    }

    /* Check watchdog timeout */
    if (HWD_EIP_IsWatchdogTimeout(handle)) {
        handle->connection_active = false;
        return SERVO_TIMEOUT;
    }

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_GetCommand(HWD_EIP_Handle_t* handle, HWD_EIP_Command_t* command) {
    if (handle == NULL || command == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    if (!g_command_received) {
        return SERVO_ERROR; /* No new command */
    }

    /* Copy command */
    memcpy(command, &g_last_command, sizeof(HWD_EIP_Command_t));

    /* Update watchdog */
    handle->last_command_time_ms = g_last_command_time_ms;
    handle->connection_active = true;

    g_command_received = false; /* Mark as consumed */

    return SERVO_OK;
}

Servo_Status_t HWD_EIP_SetTelemetry(
    HWD_EIP_Handle_t* handle,
    const HWD_EIP_Telemetry_Header_t* header,
    const int32_t* position,
    const float* velocity,
    const float* current,
    const float* voltage,
    const float* temperature
) {
    if (handle == NULL || header == NULL) {
        return SERVO_ERROR_NULL_PTR;
    }

    /* Pack telemetry into assembly buffer */
    return HWD_EIP_Assembly_PackTelemetry(
        g_input_assembly_data,
        &g_assembly_layout,
        header,
        position,
        velocity,
        current,
        voltage,
        temperature
    );
}

bool HWD_EIP_IsWatchdogTimeout(HWD_EIP_Handle_t* handle) {
    if (handle == NULL || !handle->connection_active) {
        return false;
    }

    uint32_t current_time = HWD_Timer_GetMillis();
    uint32_t elapsed = current_time - handle->last_command_time_ms;

    if (elapsed > handle->config.watchdog_timeout_ms) {
        handle->timeout_count++;
        return true;
    }

    return false;
}

void HWD_EIP_ResetWatchdog(HWD_EIP_Handle_t* handle) {
    if (handle == NULL) {
        return;
    }

    handle->last_command_time_ms = HWD_Timer_GetMillis();
    handle->connection_active = false;
}

void HWD_EIP_Shutdown(HWD_EIP_Handle_t* handle) {
    if (handle == NULL) {
        return;
    }

    /* Shutdown network handler */
    NetworkHandlerFinish();

    handle->initialized = false;
    handle->connection_active = false;
}

uint16_t HWD_EIP_GetTelemetrySize(const HWD_EIP_Handle_t* handle) {
    if (handle == NULL) {
        return 0;
    }
    return handle->telemetry_size;
}

bool HWD_EIP_IsConnected(const HWD_EIP_Handle_t* handle) {
    if (handle == NULL) {
        return false;
    }
    return handle->connection_active;
}

#endif /* USE_HWD_EIP */
