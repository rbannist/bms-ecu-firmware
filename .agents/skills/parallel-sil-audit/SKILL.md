---
name: parallel-sil-audit
description: Delegate parallel verification tasks to three specialised Antigravity subagents (MISRA Static Compliance Auditor, ELF Memory Footprint Inspector, and SIL Test Bench Validator) and synthesise a unified ISO 26262 Verification Artifact. Trigger on "/parallel-sil-audit", "run parallel subagent verification", or "audit BMS ECU with subagents".
---

# Parallel Subagent Verification Skill (`parallel-sil-audit`)

Use this skill to demonstrate **Stage 2: Deterministic Local Verification with Antigravity Subagents**.

## Execution Protocol

1. **Spawn Three Parallel Subagents (`invoke_subagent`):**
   Launch three concurrent `self` (or `research`) subagents in a single `invoke_subagent` call:
   - **Subagent 1 — `MISRA-C:2012 Static Auditor`:**
     - *Task:* Run `python3 scripts/misra_check.py`, inspect `src/bms_fault_monitor.c` and `include/*.h` against `docs/REQ-BMS-042-overcurrent.md`, and return a structured report confirming zero dynamic heap usage (`malloc`/`free`), `<stdint.h>` fixed-width type compliance, and defensive pointer checks.
   - **Subagent 2 — `ELF Memory Footprint Inspector`:**
     - *Task:* Run `make footprint` (and `nm -S build/bms_fault_monitor.o` / `size build/bms_fault_monitor.o`), verify `.text` $\le 2048\text{ B}$ and `.data + .bss` $\le 64\text{ B}$, and return the exact symbol table and memory budget breakdown.
   - **Subagent 3 — `SIL Test Bench & Timing Validator`:**
     - *Task:* Run `make test`, verify all 5 Software-in-the-Loop (SIL) CAN bus test vectors in `tests/sil_harness.c`, confirm the 50 ms (5-tick) Fault Tolerant Time Interval (FTTI) requirement `REQ-BMS-042.2`, and return the vector-by-vector timing verification results.

2. **Synthesise Verification Artifact:**
   Once all three subagents report back, compile their findings into a single structured Verification Artifact (`bms_sil_verification_report.md`) containing:
   - Executive Compliance Status (`PASS` / `FAIL`)
   - MISRA-C:2012 & Zero-Heap Audit Table
   - Static RAM/ROM Footprint Breakdown
   - SIL Test Vector & 50 ms FTTI Timing Matrix
