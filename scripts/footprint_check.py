#!/usr/bin/env python3
"""Static ELF memory footprint & zero-heap symbol verifier.

Inspects the compiled ECU object file (build/bms_fault_monitor.o) using `size`
and `nm` to verify:
1. Zero undefined references to dynamic allocation symbols (malloc, calloc, realloc, free).
2. Static RAM (.data + .bss) <= 64 bytes.
3. Program ROM (.text) <= 2048 bytes.
"""

from __future__ import annotations

import pathlib
import subprocess
import sys

MAX_TEXT_BYTES = 2048
MAX_STATIC_RAM_BYTES = 64
FORBIDDEN_SYMBOLS = {"malloc", "calloc", "realloc", "free", "aligned_alloc"}


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    obj_path = repo_root / "build" / "bms_fault_monitor.o"

    if not obj_path.exists():
        print(f"[ERROR] Object file not found at {obj_path}. Run `make build` first.")
        return 1

    print("============================================================")
    print(" ECU ELF Memory Footprint & Symbol Audit")
    print("============================================================")

    # 1. Check symbol table with `nm`
    nm_res = subprocess.run(["nm", str(obj_path)], capture_output=True, text=True, check=True)
    for line in nm_res.stdout.splitlines():
        parts = line.split()
        if parts:
            sym = parts[-1].lstrip("_")
            if sym in FORBIDDEN_SYMBOLS:
                print(f"  [FAIL] Forbidden dynamic allocation symbol linked: {sym}")
                return 1
    print("  [PASS] Symbol table verified: 0 heap allocation symbols (malloc/free)")

    # 2. Check section sizes with `size`
    size_res = subprocess.run(["size", str(obj_path)], capture_output=True, text=True, check=True)
    lines = [ln.strip() for ln in size_res.stdout.splitlines() if ln.strip()]
    if len(lines) < 2:
        print("[ERROR] Unexpected output from `size` command.")
        return 1

    fields = lines[1].split()
    text_bytes = int(fields[0])
    data_bytes = int(fields[1])
    bss_bytes = int(fields[2])
    static_ram = data_bytes + bss_bytes

    print(f"  [INFO] ROM (.text):        {text_bytes:4d} / {MAX_TEXT_BYTES} bytes")
    print(
        f"  [INFO] RAM (.data + .bss): {static_ram:4d} / {MAX_STATIC_RAM_BYTES} bytes (.data={data_bytes}, .bss={bss_bytes})"
    )

    if text_bytes > MAX_TEXT_BYTES:
        print(f"  [FAIL] ROM footprint ({text_bytes} B) exceeds budget ({MAX_TEXT_BYTES} B)")
        return 1
    if static_ram > MAX_STATIC_RAM_BYTES:
        print(
            f"  [FAIL] Static RAM footprint ({static_ram} B) exceeds budget ({MAX_STATIC_RAM_BYTES} B)"
        )
        return 1

    print("------------------------------------------------------------")
    print(" Memory Footprint Result: PASS (Within ASIL-C ECU budget)")
    print("============================================================")
    return 0


if __name__ == "__main__":
    sys.exit(main())
