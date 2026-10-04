/**
 * @file bms_fault_monitor.h
 * @brief Public interface for the BMS Overcurrent Fault Monitor (REQ-BMS-042).
 *
 * Compliant with MISRA-C:2012 subset: fixed-width <stdint.h> types only,
 * zero dynamic memory allocation, and deterministic execution time.
 */

#ifndef BMS_FAULT_MONITOR_H
#define BMS_FAULT_MONITOR_H

#include <stdbool.h>
#include <stdint.h>
#include "bms_hw_regs.h"

/** @brief Cyclic task execution period in milliseconds (10 ms). */
#define BMS_TASK_PERIOD_MS          (10U)

/** @brief Overcurrent threshold in milliamperes (500 A = 500,000 mA). */
#define BMS_OVERCURRENT_LIMIT_MA    (500000UL)

/**
 * @brief Number of consecutive 10ms overcurrent ticks required to trip (50 ms FTTI).
 * Per REQ-BMS-042.2, the contactor must open on the 5th consecutive tick.
 */
#define BMS_DEBOUNCE_LIMIT_TICKS    (5U)

/**
 * @brief Finite state machine states for the BMS Overcurrent Fault Monitor.
 */
typedef enum {
    BMS_STATE_NORMAL = 0U,        /**< Nominal operation; contactor closed */
    BMS_STATE_DEBOUNCING = 1U,    /**< Overcurrent detected; counting consecutive ticks */
    BMS_STATE_FAULT_LATCHED = 2U  /**< Fault latched; contactor opened until reset */
} BMS_MonitorStateEnum_t;

/**
 * @brief Caller-allocated state structure (zero heap allocation).
 */
typedef struct {
    BMS_MonitorStateEnum_t current_state; /**< Active state machine state */
    uint8_t debounce_ticks;               /**< Consecutive overcurrent tick counter */
    uint32_t last_recorded_current_ma;    /**< Last sampled pack current in mA */
    bool contactor_tripped;               /**< True when high-voltage contactor is tripped */
} BMS_MonitorState_t;

/**
 * @brief Initialises the BMS fault monitor state and closes the contactor.
 *
 * @param[out] state Pointer to caller-allocated monitor state structure.
 * @param[out] regs  Pointer to memory-mapped (or SIL-mocked) hardware registers.
 * @return true if initialisation succeeded, false if any pointer was NULL.
 */
bool BMS_FaultMonitor_Init(BMS_MonitorState_t *state, BMS_HwRegs_t *regs);

/**
 * @brief Executes one deterministic 10ms step of the BMS overcurrent monitor.
 *
 * @param[in,out] state Pointer to caller-allocated monitor state structure.
 * @param[in,out] regs  Pointer to memory-mapped (or SIL-mocked) hardware registers.
 * @return Current state of the monitor after evaluating the step.
 */
BMS_MonitorStateEnum_t BMS_FaultMonitor_Step(BMS_MonitorState_t *state, BMS_HwRegs_t *regs);

#endif /* BMS_FAULT_MONITOR_H */
