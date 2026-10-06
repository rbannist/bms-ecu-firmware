# PHEV/HEV High-Voltage Battery Pack & HPCU Contactor Overcurrent ECU Firmware (`bms-ecu-firmware`)

Deterministic C99 embedded controller module and Software-in-the-Loop (SIL) verification suite implementing **`REQ-BMS-042`** (ISO 26262 ASIL-C 50ms Fault Tolerant Time Interval 200 A overcurrent protection for 200–350V hybrid powertrains).

## Quick Verification Commands

```bash
# Run full compliance & SIL suite (MISRA-C subset + ELF memory footprint + SIL test bench)
make check

# Run individual verification stages
make misra       # Static MISRA-C:2012 subset & zero-heap check
make footprint   # ELF symbol & RAM/ROM budget audit
make test        # Compile & execute the 5 SIL test vectors
make arm-cross   # Optional ARM Cortex-M4 cross-compilation check (if arm-none-eabi-gcc installed)
```

## Repository Structure

- [`docs/REQ-BMS-042-overcurrent.md`](./docs/REQ-BMS-042-overcurrent.md) — Simulink-derived state machine specification & register map.
- [`include/bms_hw_regs.h`](./include/bms_hw_regs.h) — Memory-mapped hardware register definitions (`0x40021000`).
- [`include/bms_fault_monitor.h`](./include/bms_fault_monitor.h) — Public ECU application interface.
- [`src/bms_fault_monitor.c`](./src/bms_fault_monitor.c) — Deterministic C99 implementation.
- [`tests/sil_harness.c`](./tests/sil_harness.c) — Software-in-the-Loop (SIL) test bench.
- [`scripts/misra_check.py`](./scripts/misra_check.py) — Static MISRA-C:2012 subset & zero-heap verifier.
- [`scripts/footprint_check.py`](./scripts/footprint_check.py) — ELF object `.text`/`.data`/`.bss` budget verifier.
- [`.agents/skills/`](./.agents/skills/) — Antigravity skills (`/verify-misra`, `/build-sil`, `/parallel-sil-audit`).
