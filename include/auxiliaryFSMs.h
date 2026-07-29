/// @file auxiliaryFSMs.h
/// Auxiliary FSM logic for Faraday cup mechanisms.
/// Pure domain logic without hardware dependencies.

#ifndef AUXILIARY_FSMS_H
#define AUXILIARY_FSMS_H

#include <stdbool.h>
#include <stdint.h>
#include "configFaradayCups.h"

#define AUXILIARY_FSM_ERROR_TIMEOUT_INSERT                 0x0001u
#define AUXILIARY_FSM_ERROR_TIMEOUT_WITHDRAW               0x0002u
#define AUXILIARY_FSM_ERROR_UNSUPPORTED_CONFIG             0x0004u
#define AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC            0x0008u
#define AUXILIARY_FSM_ERROR_SWITCH_OF_PNEUMATIC_WITH_LOCK  0x0010u
#define AUXILIARY_FSM_ERROR_SWITCHES_OF_MOTOR_ACTUATOR     0x0020u
#define AUXILIARY_FSM_ERROR_SWITCH_A_OF_MOTOR_ACTUATOR     0x0040u
#define AUXILIARY_FSM_ERROR_SWITCH_B_OF_MOTOR_ACTUATOR     0x0080u

typedef enum {
    PNEUMATIC_FSM_STATE_BOOTED = 0,
    PNEUMATIC_FSM_STATE_EXTRACTED,  // stable
    PNEUMATIC_FSM_STATE_INSERTING,
    PNEUMATIC_FSM_STATE_INSERTED,   // stable
    PNEUMATIC_FSM_STATE_WITHDRAWING,
    PNEUMATIC_FSM_STATE_UNDEFINED
} PneumaticFsmStateEnum;

typedef enum {
    PNEUMATIC_WITH_LOCK_FSM_STATE_BOOTED = 0,
    PNEUMATIC_WITH_LOCK_FSM_STATE_EXTRACTED,  // stable
    PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTING,
    PNEUMATIC_WITH_LOCK_FSM_STATE_INSERTED,   // stable
    PNEUMATIC_WITH_LOCK_FSM_STATE_WITHDRAWING,
    PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_LOCK,
    PNEUMATIC_WITH_LOCK_FSM_STATE_LOCKED_INSERTED,  // stable
    PNEUMATIC_WITH_LOCK_FSM_STATE_PAUSE_AFTER_UNLOCK,
    PNEUMATIC_WITH_LOCK_FSM_STATE_UNDEFINED
} PneumaticWithLockFsmStateEnum;

typedef enum {
    MOTOR_FSM_STATE_BOOTED = 0,
    MOTOR_FSM_STATE_INSERTED,   // stable
    MOTOR_FSM_STATE_WITHDRAWING,
    MOTOR_FSM_STATE_WITHDRAWING_PRE_BRAKING,
    MOTOR_FSM_STATE_WITHDRAWING_BRAKING,
    MOTOR_FSM_STATE_EXTRACTED,  // stable
    MOTOR_FSM_STATE_INSERTING,
    MOTOR_FSM_STATE_INSERTING_PRE_BRAKING,
    MOTOR_FSM_STATE_INSERTING_BRAKING,
    MOTOR_FSM_STATE_UNDEFINED
} MotorFsmStateEnum;

typedef struct {
    uint16_t installed_cups;
    bool external_inhibition;

    uint16_t cup_type[MAX_CUPS];

    uint16_t time_limit_inserting_ms[MAX_CUPS];
    uint16_t time_limit_withdrawing_ms[MAX_CUPS];

    bool cup_requested_state[MAX_CUPS];

    bool cup_switch[MAX_CUPS];
    bool cup_switch_a[MAX_CUPS];
    bool cup_switch_b[MAX_CUPS];
} AuxiliaryFSMsInputs;

typedef struct {
    bool actuator_insert[MAX_CUPS];
    bool actuator_withdraw[MAX_CUPS];
    bool actuator_brake[MAX_CUPS];

    bool trigger_insert[MAX_CUPS];
    bool trigger_withdraw[MAX_CUPS];
    bool trigger_brake[MAX_CUPS];

    uint16_t cup_error[MAX_CUPS];
} AuxiliaryFSMsOutputs;

typedef struct {
    PneumaticFsmStateEnum pneumatic_fsm_state[MAX_CUPS];
    PneumaticWithLockFsmStateEnum pneumatic_with_lock_fsm_state[MAX_CUPS];
    MotorFsmStateEnum motor_fsm_state[MAX_CUPS];

    uint16_t pause_after_boot_elapsed[MAX_CUPS];
    uint16_t transition_elapsed[MAX_CUPS];
    uint16_t pause_after_lock_elapsed[MAX_CUPS];
    uint16_t pause_after_unlock_elapsed[MAX_CUPS];
    uint16_t pre_braking_elapsed[MAX_CUPS];
    uint16_t braking_elapsed[MAX_CUPS];
    bool is_cup_inserted[MAX_CUPS];
    uint16_t active_cup;
} AuxiliaryFSMsState;

void auxiliaryFSMsTick(const AuxiliaryFSMsInputs *Inputs,
                       AuxiliaryFSMsState *FsmState,
                       AuxiliaryFSMsOutputs *Outputs);

#endif // AUXILIARY_FSMS_H
