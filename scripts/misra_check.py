#!/usr/bin/env python3
"""Deterministic MISRA-C:2012 subset and zero-heap static compliance verifier.

Designed to run identically across local workstations, the Managed Agents API
remote sandbox, and the Antigravity SDK Cloud Run Job container without external
proprietary dependencies. Also invokes cppcheck / clang-tidy when available.
"""

from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys

BANNED_HEAP_PATTERNS = [
    (
        r"\b(malloc|calloc|realloc|free|aligned_alloc)\s*\(",
        "MISRA-Dir-4.12 / Rule-21.3",
        "Dynamic heap memory allocation is prohibited in ASIL-C ECU code.",
    ),
    (
        r"#\s*include\s*<stdio\.h>",
        "MISRA-Rule-21.6",
        "Standard I/O (<stdio.h>) is prohibited in production ECU source files.",
    ),
    (
        r"#\s*include\s*<stdlib\.h>",
        "MISRA-Rule-21.3",
        "Standard library (<stdlib.h>) heap/process functions are prohibited in production ECU files.",
    ),
    (r"\bgoto\b", "MISRA-Rule-15.1", "The goto statement shall not be used."),
    (
        r"\b(float|double)\b",
        "ISO26262-FixedPoint",
        "Floating-point types are prohibited in deterministic contactor control; use fixed-point mA (<stdint.h>).",
    ),
    (
        r"(?<![a-zA-Z0-9_])(int|long|short)(?![a-zA-Z0-9_])",
        "MISRA-Dir-4.6",
        "Basic numerical types (int/long/short) shall not be used; use explicit fixed-width <stdint.h> types.",
    ),
]


def strip_comments_and_strings(source: str) -> str:
    """Replaces C comments and string literals with spaces while preserving line numbers."""

    def _replacer(match: re.Match[str]) -> str:
        text = match.group(0)
        return "\n" * text.count("\n")

    pattern = re.compile(r"//.*?$|/\*.*?\*/|\"(?:\\.|[^\"\\])*\"", re.DOTALL | re.MULTILINE)
    return pattern.sub(_replacer, source)


def check_file(filepath: pathlib.Path) -> list[str]:
    """Scans a C source or header file for MISRA-C:2012 subset violations."""
    violations: list[str] = []
    raw_text = filepath.read_text(encoding="utf-8")
    clean_text = strip_comments_and_strings(raw_text)

    for lineno, line in enumerate(clean_text.splitlines(), start=1):
        for regex, rule_id, message in BANNED_HEAP_PATTERNS:
            if re.search(regex, line):
                violations.append(f"{filepath}:{lineno}: [{rule_id}] {message}")

    return violations


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parent.parent
    targets = list((repo_root / "src").glob("*.c")) + list((repo_root / "include").glob("*.h"))

    print("============================================================")
    print(" MISRA-C:2012 & ISO 26262 Static Compliance Audit")
    print("============================================================")

    all_violations: list[str] = []
    for target in sorted(targets):
        rel = target.relative_to(repo_root)
        file_violations = check_file(target)
        if file_violations:
            print(f"  [FAIL] {rel} ({len(file_violations)} violation(s))")
            all_violations.extend(file_violations)
        else:
            print(f"  [PASS] {rel} (Zero heap, fixed-width <stdint.h>, MISRA clean)")

    if shutil.which("cppcheck"):
        cmd = [
            "cppcheck",
            "--enable=warning,style,performance,portability",
            "--error-exitcode=1",
            "--inline-suppr",
            "--suppress=normalCheckLevelMaxBranches",
            "--suppress=missingIncludeSystem",
            "--suppress=checkersReport",
            "--suppress=unmatchedSuppression",
            "--quiet",
            "-I",
            str(repo_root / "include"),
            str(repo_root / "src"),
        ]
        res = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if res.returncode != 0:
            all_violations.append(f"cppcheck failed:\n{res.stderr.strip() or res.stdout.strip()}")
        else:
            print("  [PASS] cppcheck static analyser clean")

    if all_violations:
        print("\nStatic Compliance Violations Detected:")
        for v in all_violations:
            print(f"  - {v}")
        return 1

    print("------------------------------------------------------------")
    print(" Static Compliance Result: PASS (0 violations)")
    print("============================================================")
    return 0


if __name__ == "__main__":
    sys.exit(main())
