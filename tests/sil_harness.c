/**
 * @file sil_harness.c
 * @brief Software-in-the-Loop (SIL) Verification Test Bench for REQ-BMS-042.
 *
 * Simulates 10ms cyclic CAN/ADC current vectors against the BMS Overcurrent
 * Fault Monitor and verifies state transitions, contactor control register
 * bits, and the 50ms (5-tick) Fault Tolerant Time Interval (FTTI).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bms_fault_monitor.h"

static uint32_t g_tests_run = 0U;
static uint32_t g_tests_failed = 0U;

#define SIL_ASSERT(cond, msg)                                                      \
    do {                                                                           \
        if (!(cond)) {                                                             \
            printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, (msg));             \
            return false;                                                          \
        }                                                                          \
    } while (0)

/**
 * @brief Vector 1: Nominal HEV pack current (120 A assist/charge) keeps contactor closed.
 */
static bool test_normal_operation_below_threshold(void) {
    BMS_MonitorState_t state;
    BMS_HwRegs_t regs;

    SIL_ASSERT(BMS_FaultMonitor_Init(&state, &regs), "Init should succeed");

    for (uint8_t tick = 1U; tick <= 10U; tick++) {
        regs.pack_current_ma = 120000UL; /* 120 A */
        BMS_MonitorStateEnum_t s = BMS_FaultMonitor_Step(&state, &regs);
        SIL_ASSERT(s == BMS_STATE_NORMAL, "State must remain BMS_STATE_NORMAL at 120A");
        SIL_ASSERT(state.debounce_ticks == 0U, "Debounce counter must remain 0");
        SIL_ASSERT(regs.contactor_ctrl == BMS_REG_CONTACTOR_CLOSED_BIT, "Contactor must stay closed");
        SIL_ASSERT(regs.fault_status == BMS_FAULT_NONE, "No fault flags should be set");
    }
    return true;
}

/**
 * @brief Vector 2: Transient ICE starter-generator (ISG) crank assist / regen pulse (4 ticks @ 240 A = 40ms < 50ms FTTI) is rejected.
 */
static bool test_transient_spike_rejection(void) {
    BMS_MonitorState_t state;
    BMS_HwRegs_t regs;

    SIL_ASSERT(BMS_FaultMonitor_Init(&state, &regs), "Init should succeed");

    /* 4 consecutive ticks (40ms) above 200A threshold */
    for (uint8_t tick = 1U; tick <= 4U; tick++) {
        regs.pack_current_ma = 240000UL; /* 240 A */
        BMS_MonitorStateEnum_t s = BMS_FaultMonitor_Step(&state, &regs);
        SIL_ASSERT(s == BMS_STATE_DEBOUNCING, "Must be debouncing during ticks 1..4");
        SIL_ASSERT(state.debounce_ticks == tick, "Debounce tick counter mismatch");
        SIL_ASSERT(regs.contactor_ctrl == BMS_REG_CONTACTOR_CLOSED_BIT, "Contactor must remain closed during debounce");
    }

    /* Tick 5 drops back to nominal 120A -> must reset to NORMAL without tripping */
    regs.pack_current_ma = 120000UL;
    BMS_MonitorStateEnum_t s = BMS_FaultMonitor_Step(&state, &regs);
    SIL_ASSERT(s == BMS_STATE_NORMAL, "Must recover to BMS_STATE_NORMAL when transient clears on tick 5");
    SIL_ASSERT(state.debounce_ticks == 0U, "Debounce ticks must reset to 0");
    SIL_ASSERT(!state.contactor_tripped, "Contactor must not have tripped");
    SIL_ASSERT(regs.contactor_ctrl == BMS_REG_CONTACTOR_CLOSED_BIT, "Contactor must still be closed");
    return true;
}

/**
 * @brief Vector 3: Sustained overcurrent (5 ticks @ 240 A = 50ms FTTI during inverter short / boost DC-DC fault) trips contactor on tick 5.
 * Per REQ-BMS-042.2, the contactor MUST trip at 50ms (tick 5), NOT 60ms (tick 6).
 */
static bool test_sustained_overcurrent_trips_at_50ms(void) {
    BMS_MonitorState_t state;
    BMS_HwRegs_t regs;

    SIL_ASSERT(BMS_FaultMonitor_Init(&state, &regs), "Init should succeed");

    /* Ticks 1..4 (10ms..40ms): Debouncing */
    for (uint8_t tick = 1U; tick <= 4U; tick++) {
        regs.pack_current_ma = 240000UL;
        BMS_MonitorStateEnum_t s = BMS_FaultMonitor_Step(&state, &regs);
        SIL_ASSERT(s == BMS_STATE_DEBOUNCING, "Ticks 1..4 must be in BMS_STATE_DEBOUNCING");
    }

    /* Tick 5 (50ms FTTI boundary): MUST trip and latch fault immediately */
    regs.pack_current_ma = 240000UL;
    BMS_MonitorStateEnum_t s5 = BMS_FaultMonitor_Step(&state, &regs);
    SIL_ASSERT(
        s5 == BMS_STATE_FAULT_LATCHED,
        "REQ-BMS-042.2 VIOLATION: Contactor failed to latch fault at 50ms (Tick 5)! FTTI exceeded."
    );
    SIL_ASSERT(state.contactor_tripped, "contactor_tripped flag must be true at 50ms (Tick 5)");
    SIL_ASSERT(
        regs.contactor_ctrl == BMS_REG_CONTACTOR_TRIP_BIT,
        "Hardware contactor register must assert TRIP_BIT at 50ms (Tick 5)"
    );
    SIL_ASSERT(
        (regs.fault_status & BMS_FAULT_OVERCURRENT_BIT) != 0UL,
        "Overcurrent fault status bit must be set at 50ms (Tick 5)"
    );
    return true;
}

/**
 * @brief Vector 4: Latched fault remains latched even after pack current drops to 0 A.
 */
static bool test_fault_latch_persists_after_current_drops(void) {
    BMS_MonitorState_t state;
    BMS_HwRegs_t regs;

    SIL_ASSERT(BMS_FaultMonitor_Init(&state, &regs), "Init should succeed");

    for (uint8_t tick = 1U; tick <= 5U; tick++) {
        regs.pack_current_ma = 280000UL;
        (void)BMS_FaultMonitor_Step(&state, &regs);
    }
    SIL_ASSERT(state.current_state == BMS_STATE_FAULT_LATCHED, "Must be latched after 5 ticks at 280A");

    /* Drop current to 0 A; contactor must NOT re-close automatically */
    regs.pack_current_ma = 0UL;
    BMS_MonitorStateEnum_t s_after = BMS_FaultMonitor_Step(&state, &regs);
    SIL_ASSERT(s_after == BMS_STATE_FAULT_LATCHED, "REQ-BMS-042.4: Fault must stay latched when current drops");
    SIL_ASSERT(regs.contactor_ctrl == BMS_REG_CONTACTOR_TRIP_BIT, "Contactor must remain open while latched");
    return true;
}

/**
 * @brief Vector 5: Sensor open-circuit diagnostic (0xFFFFFFFFU) and NULL pointer safety.
 */
static bool test_null_pointer_and_sensor_fault_safety(void) {
    BMS_MonitorState_t state;
    BMS_HwRegs_t regs;

    SIL_ASSERT(!BMS_FaultMonitor_Init(NULL, &regs), "Init with NULL state must return false");
    SIL_ASSERT(!BMS_FaultMonitor_Init(&state, NULL), "Init with NULL regs must return false");

    SIL_ASSERT(BMS_FaultMonitor_Init(&state, &regs), "Valid init should succeed");
    regs.pack_current_ma = BMS_SENSOR_INVALID_RAW;
    BMS_MonitorStateEnum_t s = BMS_FaultMonitor_Step(&state, &regs);
    SIL_ASSERT(s == BMS_STATE_FAULT_LATCHED, "REQ-BMS-042.5: Sensor fault must immediately latch fault state");
    SIL_ASSERT((regs.fault_status & BMS_FAULT_SENSOR_ERR_BIT) != 0UL, "Sensor error bit must be asserted");
    SIL_ASSERT(regs.contactor_ctrl == BMS_REG_CONTACTOR_TRIP_BIT, "Contactor must trip on sensor fault");
    return true;
}

int main(void) {
    typedef struct {
        const char *name;
        bool (*fn)(void);
    } TestEntry_t;

    const TestEntry_t suite[] = {
        {"test_normal_operation_below_threshold", test_normal_operation_below_threshold},
        {"test_transient_spike_rejection", test_transient_spike_rejection},
        {"test_sustained_overcurrent_trips_at_50ms", test_sustained_overcurrent_trips_at_50ms},
        {"test_fault_latch_persists_after_current_drops", test_fault_latch_persists_after_current_drops},
        {"test_null_pointer_and_sensor_fault_safety", test_null_pointer_and_sensor_fault_safety},
    };

    printf("============================================================\n");
    printf(" BMS/HPCU Overcurrent Fault Monitor — SIL Verification Harness\n");
    printf(" Target Spec: REQ-BMS-042 (ISO 26262 ASIL-C, 50ms FTTI, 200A limit)\n");
    printf("============================================================\n");

    const uint32_t total = (uint32_t)(sizeof(suite) / sizeof(suite[0]));
    for (uint32_t i = 0U; i < total; i++) {
        g_tests_run++;
        if (suite[i].fn()) {
            printf("  [PASS] %s\n", suite[i].name);
        } else {
            g_tests_failed++;
        }
    }

    printf("------------------------------------------------------------\n");
    printf(" SIL Summary: %u/%u passed, %u failed\n",
           (unsigned int)(g_tests_run - g_tests_failed),
           (unsigned int)g_tests_run,
           (unsigned int)g_tests_failed);
    printf("============================================================\n");

    return (g_tests_failed == 0U) ? 0 : 1;
}
