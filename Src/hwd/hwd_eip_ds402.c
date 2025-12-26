/*******************************************************************************
 * @file    hwd_eip_ds402.c
 * @brief   DS402 CANopen Drive Profile State Machine Implementation
 * @details Повна реалізація DS402 state machine для servo drives
 *
 * @author  ServoLib Team
 * @date    26.12.2024
 ******************************************************************************/

#include "hwd/hwd_eip_ds402.h"

#ifdef USE_HWD_EIP

#include <string.h>

/*******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

/**
 * @brief Перевірка Control Word command
 * @param control_word  Control Word від Controller
 * @param mask          Command mask
 * @param expected      Expected value
 * @return bool         true якщо command matches
 */
static inline bool ds402_check_command(uint16_t control_word, uint16_t mask, uint16_t expected) {
    return ((control_word & mask) == expected);
}

/**
 * @brief State transition handler
 * @param sm            State machine
 * @param new_state     New state
 */
static void ds402_transition_to(DS402_StateMachine_t* sm, DS402_State_t new_state) {
    if (sm->current_state != new_state) {
        sm->previous_state = sm->current_state;
        sm->current_state = new_state;
        sm->transition_count++;

        /* Update motor_enabled flag */
        sm->motor_enabled = (new_state == DS402_STATE_OPERATION_ENABLED);
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

void DS402_Init(DS402_StateMachine_t* sm) {
    if (sm == NULL) {
        return;
    }

    memset(sm, 0, sizeof(DS402_StateMachine_t));

    /* Початковий стан: Not Ready to Switch On */
    sm->current_state = DS402_STATE_NOT_READY_TO_SWITCH_ON;
    sm->previous_state = DS402_STATE_NOT_READY_TO_SWITCH_ON;
    sm->motor_enabled = false;
    sm->fault_condition = false;
}

DS402_State_t DS402_ProcessControlWord(
    DS402_StateMachine_t* sm,
    uint16_t control_word,
    bool fault_cond
) {
    if (sm == NULL) {
        return DS402_STATE_NOT_READY_TO_SWITCH_ON;
    }

    /* Зберегти fault condition */
    sm->fault_condition = fault_cond;

    /* Перевірити Fault Reset command */
    bool fault_reset = (control_word & DS402_CMD_FAULT_RESET) != 0;

    /* Fault має найвищий пріоритет */
    if (fault_cond && sm->current_state != DS402_STATE_FAULT &&
        sm->current_state != DS402_STATE_FAULT_REACTION_ACTIVE) {
        /* Перейти у Fault Reaction, потім автоматично у Fault */
        ds402_transition_to(sm, DS402_STATE_FAULT_REACTION_ACTIVE);
        sm->fault_count++;
        /* Автоматичний перехід у Fault (симулюємо одразу) */
        ds402_transition_to(sm, DS402_STATE_FAULT);
        return sm->current_state;
    }

    /* State machine transitions */
    switch (sm->current_state) {

        case DS402_STATE_NOT_READY_TO_SWITCH_ON:
            /* Автоматичний перехід у Switch On Disabled після ініціалізації */
            ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            break;

        case DS402_STATE_SWITCH_ON_DISABLED:
            /* Transition 2: Shutdown command → Ready to Switch On */
            if (ds402_check_command(control_word, 0x87, DS402_CMD_SHUTDOWN)) {
                ds402_transition_to(sm, DS402_STATE_READY_TO_SWITCH_ON);
            }
            break;

        case DS402_STATE_READY_TO_SWITCH_ON:
            /* Transition 3: Switch On command → Switched On */
            if (ds402_check_command(control_word, 0x8F, DS402_CMD_SWITCH_ON)) {
                ds402_transition_to(sm, DS402_STATE_SWITCHED_ON);
            }
            /* Transition 7: Disable Voltage → Switch On Disabled */
            else if (ds402_check_command(control_word, 0x82, DS402_CMD_DISABLE_VOLTAGE)) {
                ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            }
            break;

        case DS402_STATE_SWITCHED_ON:
            /* Transition 4: Enable Operation command → Operation Enabled */
            if (ds402_check_command(control_word, 0x8F, DS402_CMD_ENABLE_OPERATION)) {
                ds402_transition_to(sm, DS402_STATE_OPERATION_ENABLED);
            }
            /* Transition 6: Shutdown → Ready to Switch On */
            else if (ds402_check_command(control_word, 0x87, DS402_CMD_SHUTDOWN)) {
                ds402_transition_to(sm, DS402_STATE_READY_TO_SWITCH_ON);
            }
            /* Transition 10: Disable Voltage → Switch On Disabled */
            else if (ds402_check_command(control_word, 0x82, DS402_CMD_DISABLE_VOLTAGE)) {
                ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            }
            break;

        case DS402_STATE_OPERATION_ENABLED:
            /* Transition 5: Disable Operation → Switched On */
            if (ds402_check_command(control_word, 0x8F, DS402_CMD_DISABLE_OPERATION)) {
                ds402_transition_to(sm, DS402_STATE_SWITCHED_ON);
            }
            /* Transition 8: Shutdown → Ready to Switch On */
            else if (ds402_check_command(control_word, 0x87, DS402_CMD_SHUTDOWN)) {
                ds402_transition_to(sm, DS402_STATE_READY_TO_SWITCH_ON);
            }
            /* Transition 9: Disable Voltage → Switch On Disabled */
            else if (ds402_check_command(control_word, 0x82, DS402_CMD_DISABLE_VOLTAGE)) {
                ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            }
            /* Transition 11: Quick Stop → Quick Stop Active */
            else if (ds402_check_command(control_word, 0x86, DS402_CMD_QUICK_STOP)) {
                ds402_transition_to(sm, DS402_STATE_QUICK_STOP_ACTIVE);
            }
            break;

        case DS402_STATE_QUICK_STOP_ACTIVE:
            /* Transition 12: Disable Voltage → Switch On Disabled */
            if (ds402_check_command(control_word, 0x82, DS402_CMD_DISABLE_VOLTAGE)) {
                ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            }
            /* Transition 16: Enable Operation (optional - залежить від Quick Stop Option Code) */
            /* Для простоти: залишаємось у Quick Stop поки не Disable Voltage */
            break;

        case DS402_STATE_FAULT_REACTION_ACTIVE:
            /* Автоматичний перехід у Fault */
            ds402_transition_to(sm, DS402_STATE_FAULT);
            break;

        case DS402_STATE_FAULT:
            /* Transition 15: Fault Reset → Switch On Disabled */
            if (fault_reset && !sm->fault_condition) {
                /* Fault reset можливий тільки якщо fault умова очищена */
                sm->fault_code = 0;
                ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            }
            break;

        default:
            /* Unknown state - reset to Switch On Disabled */
            ds402_transition_to(sm, DS402_STATE_SWITCH_ON_DISABLED);
            break;
    }

    return sm->current_state;
}

uint16_t DS402_GetStatusWord(
    const DS402_StateMachine_t* sm,
    bool warning,
    bool target_reached,
    bool motor_stalled
) {
    if (sm == NULL) {
        return 0;
    }

    uint16_t status_word = 0;

    /* Базовий status word на основі state */
    switch (sm->current_state) {
        case DS402_STATE_NOT_READY_TO_SWITCH_ON:
            status_word = DS402_STATUS_NOT_READY;
            break;

        case DS402_STATE_SWITCH_ON_DISABLED:
            status_word = DS402_STATUS_SWITCH_DISABLED;
            break;

        case DS402_STATE_READY_TO_SWITCH_ON:
            status_word = DS402_STATUS_READY;
            break;

        case DS402_STATE_SWITCHED_ON:
            status_word = DS402_STATUS_SWITCHED_ON;
            break;

        case DS402_STATE_OPERATION_ENABLED:
            status_word = DS402_STATUS_OPERATION_EN;
            break;

        case DS402_STATE_QUICK_STOP_ACTIVE:
            status_word = DS402_STATUS_QUICK_STOP;
            break;

        case DS402_STATE_FAULT_REACTION_ACTIVE:
            status_word = DS402_STATUS_FAULT_REACT;
            break;

        case DS402_STATE_FAULT:
            status_word = DS402_STATUS_FAULT;
            break;

        default:
            status_word = DS402_STATUS_NOT_READY;
            break;
    }

    /* Додаткові біти */
    if (warning) {
        status_word |= (1 << 9);  /* Warning bit */
    }

    if (target_reached) {
        status_word |= (1 << 10); /* Target reached bit */
    }

    if (motor_stalled) {
        status_word |= (1 << 12); /* Motor stalled (custom) */
    }

    /* Voltage enabled - true для станів після Ready to Switch On */
    if (sm->current_state >= DS402_STATE_READY_TO_SWITCH_ON &&
        sm->current_state != DS402_STATE_FAULT &&
        sm->current_state != DS402_STATE_FAULT_REACTION_ACTIVE) {
        status_word |= (1 << 4);  /* Voltage enabled bit */
    }

    return status_word;
}

bool DS402_IsMotorEnabled(const DS402_StateMachine_t* sm) {
    if (sm == NULL) {
        return false;
    }
    return sm->motor_enabled;
}

bool DS402_IsFaulted(const DS402_StateMachine_t* sm) {
    if (sm == NULL) {
        return false;
    }
    return (sm->current_state == DS402_STATE_FAULT ||
            sm->current_state == DS402_STATE_FAULT_REACTION_ACTIVE);
}

void DS402_SetFault(DS402_StateMachine_t* sm, uint32_t fault_code) {
    if (sm == NULL) {
        return;
    }

    sm->fault_condition = true;
    sm->fault_code = fault_code;

    /* Transition відбудеться при наступному ProcessControlWord() */
}

void DS402_ClearFault(DS402_StateMachine_t* sm) {
    if (sm == NULL) {
        return;
    }

    sm->fault_condition = false;
    /* fault_code залишається для діагностики поки не Fault Reset */
}

const char* DS402_GetStateName(DS402_State_t state) {
    switch (state) {
        case DS402_STATE_NOT_READY_TO_SWITCH_ON:
            return "Not Ready to Switch On";
        case DS402_STATE_SWITCH_ON_DISABLED:
            return "Switch On Disabled";
        case DS402_STATE_READY_TO_SWITCH_ON:
            return "Ready to Switch On";
        case DS402_STATE_SWITCHED_ON:
            return "Switched On";
        case DS402_STATE_OPERATION_ENABLED:
            return "Operation Enabled";
        case DS402_STATE_QUICK_STOP_ACTIVE:
            return "Quick Stop Active";
        case DS402_STATE_FAULT_REACTION_ACTIVE:
            return "Fault Reaction Active";
        case DS402_STATE_FAULT:
            return "Fault";
        default:
            return "Unknown";
    }
}

void DS402_Reset(DS402_StateMachine_t* sm) {
    if (sm == NULL) {
        return;
    }

    /* Зберегти статистику */
    uint32_t saved_transition_count = sm->transition_count;
    uint32_t saved_fault_count = sm->fault_count;

    /* Reset до початкового стану */
    DS402_Init(sm);

    /* Відновити статистику */
    sm->transition_count = saved_transition_count;
    sm->fault_count = saved_fault_count;
}

#endif /* USE_HWD_EIP */
