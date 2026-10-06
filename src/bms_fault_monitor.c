/**
 * @file bms_fault_monitor.c
 * @brief Deterministic C99 implementation of the BMS Overcurrent Fault Monitor (REQ-BMS-042).
 *
 * Safety & Compliance Properties:
 * - Zero dynamic memory allocation (no malloc/calloc/realloc/free).
 * - Fixed-width integer types (<stdint.h>) throughout.
 * - Deterministic O(1) execution path with no loops or recursion.
 */

#include <stddef.h>
#include "bms_fault_monitor.h"

/**
 * @brief Internal helper to latch a fault state and open the high-voltage contactor.
 */
static void bms_latch_fault(
    BMS_MonitorState_t *state,
    BMS_HwRegs_t *regs,
    uint32_t fault_bit
) {
    state->current_state = BMS_STATE_FAULT_LATCHED;
    state->contactor_tripped = true;

    if (regs != NULL) {
        regs->contactor_ctrl = BMS_REG_CONTACTOR_TRIP_BIT;
        regs->fault_status |= fault_bit;
    }
}

bool BMS_FaultMonitor_Init(BMS_MonitorState_t *state, BMS_HwRegs_t *regs) {
    bool init_ok = false;

    if ((state != NULL) && (regs != NULL)) {
        state->current_state = BMS_STATE_NORMAL;
        state->debounce_ticks = 0U;
        state->last_recorded_current_ma = 0UL;
        state->contactor_tripped = false;

        regs->pack_current_ma = 0UL;
        regs->contactor_ctrl = BMS_REG_CONTACTOR_CLOSED_BIT;
        regs->fault_status = BMS_FAULT_NONE;
        init_ok = true;
    }

    return init_ok;
}

BMS_MonitorStateEnum_t BMS_FaultMonitor_Step(BMS_MonitorState_t *state, BMS_HwRegs_t *regs) {
    if (state == NULL) {
        if (regs != NULL) {
            regs->contactor_ctrl = BMS_REG_CONTACTOR_TRIP_BIT;
            regs->fault_status |= BMS_FAULT_SENSOR_ERR_BIT;
        }
        return BMS_STATE_FAULT_LATCHED;
    }

    if (regs == NULL) {
        bms_latch_fault(state, NULL, BMS_FAULT_SENSOR_ERR_BIT);
        return state->current_state;
    }

    /* REQ-BMS-042.4: Once latched, remain in fault state until explicit re-initialisation. */
    if (state->current_state == BMS_STATE_FAULT_LATCHED) {
        regs->contactor_ctrl = BMS_REG_CONTACTOR_TRIP_BIT;
        return BMS_STATE_FAULT_LATCHED;
    }

    const uint32_t sample_ma = regs->pack_current_ma;
    state->last_recorded_current_ma = sample_ma;

    /* REQ-BMS-042.5: Immediate fail-safe on invalid sensor diagnostic reading. */
    if (sample_ma == BMS_SENSOR_INVALID_RAW) {
        bms_latch_fault(state, regs, BMS_FAULT_SENSOR_ERR_BIT);
        return state->current_state;
    }

    /* REQ-BMS-042.1 & REQ-BMS-042.2: Evaluate overcurrent threshold and 50ms (5-tick) FTTI window. */
    if (sample_ma > BMS_OVERCURRENT_LIMIT_MA) {
        if (state->debounce_ticks < 255U) {
            state->debounce_ticks = (uint8_t)(state->debounce_ticks + 1U);
        }

        if (state->debounce_ticks > BMS_DEBOUNCE_LIMIT_TICKS) {
            bms_latch_fault(state, regs, BMS_FAULT_OVERCURRENT_BIT);
        } else {
            state->current_state = BMS_STATE_DEBOUNCING;
            regs->contactor_ctrl = BMS_REG_CONTACTOR_CLOSED_BIT;
        }
    } else {
        /* REQ-BMS-042.3: Transient spike cleared before 50ms FTTI limit; reset counter. */
        state->debounce_ticks = 0U;
        state->current_state = BMS_STATE_NORMAL;
        regs->contactor_ctrl = BMS_REG_CONTACTOR_CLOSED_BIT;
    }

    return state->current_state;
}
