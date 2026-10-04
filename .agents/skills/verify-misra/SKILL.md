---
name: verify-misra
description: Run deterministic MISRA-C:2012 subset static compliance and zero-heap ELF memory footprint verification on the BMS ECU firmware. Trigger on "/verify-misra", "check MISRA compliance", "audit heap usage", or "check ECU memory footprint".
---

# BMS ECU MISRA-C:2012 & Zero-Heap Static Compliance Skill

Use this skill whenever the developer asks to verify MISRA-C:2012 compliance, check for dynamic memory allocation (`malloc`/`free`), or inspect the static RAM/ROM footprint of the BMS ECU module.

## Execution Protocol

1. **Run Static MISRA-C:2012 Check:**
   Execute `make misra` in the repository root. This runs `scripts/misra_check.py` (and `cppcheck` if installed) against `src/*.c` and `include/*.h` to verify:
   - **MISRA Dir 4.12 / Rule 21.3:** Zero dynamic heap allocation (`malloc`, `calloc`, `realloc`, `free`).
   - **MISRA Rule 21.6:** Zero `<stdio.h>` usage in production ECU source files.
   - **MISRA Dir 4.6:** Strict use of fixed-width `<stdint.h>` types (`uint8_t`, `uint16_t`, `uint32_t`)—no raw `int`, `long`, `short`, `float`, or `double`.
   - **MISRA Rule 15.1:** Zero `goto` statements.

2. **Run ELF Symbol & Memory Footprint Audit:**
   Execute `make footprint` in the repository root. This compiles `build/bms_fault_monitor.o` with `-std=c99 -O2 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow` and runs `scripts/footprint_check.py` (`nm` and `size`) to verify:
   - Zero undefined heap symbols in the ELF symbol table.
   - ROM (`.text`) $\le 2048\text{ bytes}$.
   - Static RAM (`.data` + `.bss`) $\le 64\text{ bytes}$.

3. **Remediate Violations (If Any):**
   If either check fails, inspect the exact `file:line` reported, edit the C source to resolve the violation while preserving `REQ-BMS-042` behaviour, and re-run `make misra && make footprint` until both pass with exit code `0`.

4. **Output Compliance Summary Table:**
   Present a concise Markdown table summarising the status of each rule (`MISRA Dir 4.12`, `MISRA Dir 4.6`, `MISRA Rule 15.1`, `.text` ROM bytes, `.data + .bss` RAM bytes).
