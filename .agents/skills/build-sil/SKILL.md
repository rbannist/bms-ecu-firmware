---
name: build-sil
description: Compile the C99 BMS Overcurrent Fault Monitor and execute the Software-in-the-Loop (SIL) verification harness against REQ-BMS-042. Trigger on "/build-sil", "run SIL tests", "verify 50ms FTTI", or "test BMS state machine".
---

# BMS Software-in-the-Loop (SIL) Build & Verification Skill

Use this skill to compile the BMS Overcurrent Fault Monitor (`src/bms_fault_monitor.c`) and execute the Software-in-the-Loop (SIL) test harness (`tests/sil_harness.c`) verifying ISO 26262 ASIL-C requirement `REQ-BMS-042`.

## Execution Protocol

1. **Compile and Run the SIL Test Bench:**
   Run `make test` in the repository root.

2. **Verify All 5 CAN / ADC Simulation Vectors:**
   Confirm that `build/sil_runner` passes every test vector:
   - `test_normal_operation_below_threshold` (120 A steady state assist/charge $\rightarrow$ `BMS_STATE_NORMAL`, contactor closed).
   - `test_transient_spike_rejection` (4 ticks / 40 ms @ 240 A ISG crank assist / regen pulse followed by 120 A $\rightarrow$ transient rejected, no trip).
   - `test_sustained_overcurrent_trips_at_50ms` (5 consecutive ticks / 50 ms @ 240 A sustained inverter short / boost DC-DC fault $\rightarrow$ latches `BMS_STATE_FAULT_LATCHED` on tick 5, asserts `BMS_REG_CONTACTOR_TRIP_BIT`).
   - `test_fault_latch_persists_after_current_drops` (latched fault remains active even when current returns to 0 A).
   - `test_null_pointer_and_sensor_fault_safety` (`NULL` pointer or `0xFFFFFFFFU` sensor diagnostic code immediately latches safe fault state).

3. **Diagnose & Fix Regressions (If Any):**
   If a test vector fails, cross-reference [`docs/REQ-BMS-042-overcurrent.md`](../../docs/REQ-BMS-042-overcurrent.md), inspect `src/bms_fault_monitor.c`, apply the minimal deterministic C99 fix, and re-run `make check`.
