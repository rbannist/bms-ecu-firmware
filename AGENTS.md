# Antigravity Agent Instructions — BMS ECU Firmware (`REQ-BMS-042`)

You are operating inside an automotive ISO 26262 ASIL-C embedded C99 codebase (`bms-ecu-firmware`).

## Mandatory Safety & Engineering Invariants

1. **Specification Authority:** All state transitions, thresholds, and timing windows must conform strictly to [`docs/REQ-BMS-042-overcurrent.md`](./docs/REQ-BMS-042-overcurrent.md).
2. **Zero Dynamic Heap Allocation (MISRA Dir 4.12 / Rule 21.3):** Never use `malloc`, `calloc`, `realloc`, `free`, or `<stdlib.h>`/`<stdio.h>` in `src/` or `include/`.
3. **Fixed-Width Types Only (MISRA Dir 4.6):** Use `<stdint.h>` (`uint8_t`, `uint16_t`, `uint32_t`) and `<stdbool.h>`. Never use raw `int`, `long`, `short`, `float`, or `double` in `src/` or `include/`.
4. **50ms Fault Tolerant Time Interval (FTTI):** At a 10ms cyclic task rate (`BMS_TASK_PERIOD_MS = 10U`), sustained overcurrent (`> 500000UL` mA) **must** latch `BMS_STATE_FAULT_LATCHED` and open the contactor on the **5th consecutive tick** (`debounce_ticks >= BMS_DEBOUNCE_LIMIT_TICKS`, where `BMS_DEBOUNCE_LIMIT_TICKS = 5U`).
5. **Verification Gate:** Every code modification must pass `make check` (`make misra`, `make footprint`, and `make test`) with zero warnings or errors before completion.
