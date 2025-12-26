/*******************************************************************************
 * @file    hwd_eip_ds402.h
 * @brief   DS402 CANopen Drive Profile State Machine
 * @details Повна реалізація DS402 state machine для servo drives
 *          Використовується для EtherNet/IP communications
 *
 * @note    Compile-time enable: #define USE_HWD_EIP
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#ifndef HWD_EIP_DS402_H
#define HWD_EIP_DS402_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config_lib.h"
#include "err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef USE_HWD_EIP

/*******************************************************************************
 * DEFINES - Control Word Commands
 ******************************************************************************/

/** DS402 Control Word command masks */
#define DS402_CMD_SHUTDOWN           0x0006  /**< Shutdown (0b0xxx x110) */
#define DS402_CMD_SWITCH_ON          0x0007  /**< Switch On (0b0xxx x111) */
#define DS402_CMD_DISABLE_VOLTAGE    0x0000  /**< Disable Voltage (0b0xxx xx0x) */
#define DS402_CMD_QUICK_STOP         0x0002  /**< Quick Stop (0b0xxx x01x) */
#define DS402_CMD_DISABLE_OPERATION  0x0007  /**< Disable Operation (0b0xxx 0111) */
#define DS402_CMD_ENABLE_OPERATION   0x000F  /**< Enable Operation (0b0xxx 1111) */
#define DS402_CMD_FAULT_RESET        0x0080  /**< Fault Reset (bit 7 = 1) */

/** Control Word mask для перевірки команд */
#define DS402_CMD_MASK               0x008F  /**< Mask bits: 0,1,2,3,7 */

/*******************************************************************************
 * DEFINES - Status Word Masks
 ******************************************************************************/

/** DS402 Status Word masks для визначення стану */
#define DS402_STATUSWORD_MASK        0x006F  /**< Mask bits: 0,1,2,3,5,6 */

/** Status patterns для кожного стану */
#define DS402_STATUS_NOT_READY       0x0000  /**< Not Ready to Switch On: xxxx xxx0 0000 */
#define DS402_STATUS_SWITCH_DISABLED 0x0040  /**< Switch On Disabled: xxxx xxx1 0000 */
#define DS402_STATUS_READY           0x0021  /**< Ready to Switch On: xxxx xxx0 0001 */
#define DS402_STATUS_SWITCHED_ON     0x0023  /**< Switched On: xxxx xxx0 0011 */
#define DS402_STATUS_OPERATION_EN    0x0027  /**< Operation Enabled: xxxx xxx0 0111 */
#define DS402_STATUS_QUICK_STOP      0x0007  /**< Quick Stop Active: xxxx xxx0 0111 */
#define DS402_STATUS_FAULT_REACT     0x000F  /**< Fault Reaction Active: xxxx xxx0 1111 */
#define DS402_STATUS_FAULT           0x0008  /**< Fault: xxxx xxx0 1000 */

/*******************************************************************************
 * TYPES
 ******************************************************************************/

/**
 * @brief DS402 State Machine States
 */
typedef enum {
    DS402_STATE_NOT_READY_TO_SWITCH_ON = 0,  /**< 0. Not Ready to Switch On (initialization) */
    DS402_STATE_SWITCH_ON_DISABLED = 1,      /**< 1. Switch On Disabled */
    DS402_STATE_READY_TO_SWITCH_ON = 2,      /**< 2. Ready to Switch On */
    DS402_STATE_SWITCHED_ON = 3,             /**< 3. Switched On */
    DS402_STATE_OPERATION_ENABLED = 4,       /**< 4. Operation Enabled (робочий стан) */
    DS402_STATE_QUICK_STOP_ACTIVE = 5,       /**< 5. Quick Stop Active */
    DS402_STATE_FAULT_REACTION_ACTIVE = 6,   /**< 6. Fault Reaction Active */
    DS402_STATE_FAULT = 7                    /**< 7. Fault */
} DS402_State_t;

/**
 * @brief DS402 State Machine Handle
 */
typedef struct {
    DS402_State_t current_state;        /**< Поточний стан */
    DS402_State_t previous_state;       /**< Попередній стан (для debug) */
    uint32_t fault_code;                /**< Fault code (якщо у Fault state) */
    bool motor_enabled;                 /**< Motor enabled flag (Operation Enabled) */
    bool fault_condition;               /**< Зовнішня fault умова */
    uint32_t transition_count;          /**< Лічильник transitions (статистика) */
    uint32_t fault_count;               /**< Лічильник faults (статистика) */
} DS402_StateMachine_t;

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * @brief Ініціалізація DS402 state machine
 *
 * @param sm    Вказівник на state machine структуру
 */
void DS402_Init(DS402_StateMachine_t* sm);

/**
 * @brief Обробка Control Word та state transitions
 *
 * @param sm            Вказівник на state machine
 * @param control_word  DS402 Control Word (від Controller)
 * @param fault_cond    Зовнішня fault умова (overcurrent, overtemp, etc.)
 * @return DS402_State_t Новий стан після обробки
 *
 * @note Викликати кожен цикл @ 1 kHz
 */
DS402_State_t DS402_ProcessControlWord(
    DS402_StateMachine_t* sm,
    uint16_t control_word,
    bool fault_cond
);

/**
 * @brief Формування DS402 Status Word
 *
 * @param sm              Вказівник на state machine
 * @param warning         Warning flag
 * @param target_reached  Target reached flag
 * @param motor_stalled   Motor stalled flag (custom)
 * @return uint16_t       DS402 Status Word
 */
uint16_t DS402_GetStatusWord(
    const DS402_StateMachine_t* sm,
    bool warning,
    bool target_reached,
    bool motor_stalled
);

/**
 * @brief Перевірка чи motor може працювати
 *
 * @param sm    Вказівник на state machine
 * @return bool true якщо у стані Operation Enabled
 */
bool DS402_IsMotorEnabled(const DS402_StateMachine_t* sm);

/**
 * @brief Перевірка чи у стані Fault
 *
 * @param sm    Вказівник на state machine
 * @return bool true якщо у Fault або Fault Reaction state
 */
bool DS402_IsFaulted(const DS402_StateMachine_t* sm);

/**
 * @brief Встановити fault умову
 *
 * @param sm          Вказівник на state machine
 * @param fault_code  Fault code
 *
 * @note Переведе state machine у Fault Reaction → Fault
 */
void DS402_SetFault(DS402_StateMachine_t* sm, uint32_t fault_code);

/**
 * @brief Очистити fault (для Fault Reset)
 *
 * @param sm    Вказівник на state machine
 *
 * @note Не змінює state, тільки очищає fault_condition
 *       State transition відбудеться при наступному ProcessControlWord()
 */
void DS402_ClearFault(DS402_StateMachine_t* sm);

/**
 * @brief Отримати назву стану (для debug/logging)
 *
 * @param state  DS402 state
 * @return const char* Назва стану
 */
const char* DS402_GetStateName(DS402_State_t state);

/**
 * @brief Скинути state machine до початкового стану
 *
 * @param sm    Вказівник на state machine
 */
void DS402_Reset(DS402_StateMachine_t* sm);

#endif /* USE_HWD_EIP */

#ifdef __cplusplus
}
#endif

#endif /* HWD_EIP_DS402_H */
