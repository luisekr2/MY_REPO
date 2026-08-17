#!/usr/bin/env python3
"""Extract comparable ROB-compression metrics from XiangShan emulator logs."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


FINAL_RE = re.compile(
    r"Core-0 instrCnt =\s*([0-9,]+), cycleCnt =\s*([0-9,]+), IPC =\s*([0-9.]+)"
)
PERF_RE = re.compile(r":\s+([A-Za-z0-9_]+),\s*([0-9]+)\s*$")
METRICS = (
    "branchMispredicts",
    "resolve_branch_mispredicts",
    "commit_branch_mispredicts",
    "rob_entries_saved",
    "rob_control_flow_shared_entry",
    "rob_compressed_groups_with_divsqrt",
    "rob_compressed_groups_with_mul",
    "rob_compressed_groups_cross_ftq",
    "rob_entries_saved_with_divsqrt",
    "rob_entries_saved_cross_ftq",
    "read_free_commit_stale_drop",
    "train_stall",
    "read_conflict",
    "waitDivCycle",
    "waitMulCycle",
    *(f"commitCompressCnt{size}" for size in range(1, 9)),
    *(f"waitNotWritebackedCompressSize{size}" for size in range(1, 9)),
)


def parse_log(label: str, path: Path) -> dict[str, str | int]:
    text = path.read_text(errors="replace")
    final_matches = list(FINAL_RE.finditer(text))
    if not final_matches:
        raise ValueError(f"missing final IPC line: {path}")

    final = final_matches[-1]
    row: dict[str, str | int] = {
        "label": label,
        "log": str(path),
        "good_trap": int(
            "HIT GOOD TRAP" in text
            and "HIT BAD TRAP" not in text
            and "HIT CRITICAL ERROR" not in text
            and "Assertion failed" not in text
            and "ABORT at pc" not in text
        ),
        "critical_error": int("HIT CRITICAL ERROR" in text),
        "assertion_failed": int("Assertion failed" in text),
        "aborted": int("ABORT at pc" in text),
        "limit_reached": int("EXCEEDING CYCLE/INSTR LIMIT" in text),
        "instructions": final.group(1).replace(",", ""),
        "cycles": final.group(2).replace(",", ""),
        "ipc": final.group(3),
    }

    counters: dict[str, str] = {}
    for line in text.splitlines():
        match = PERF_RE.search(line)
        if match:
            counters[match.group(1)] = match.group(2)
    for metric in METRICS:
        row[metric] = counters.get(metric, "")
    return row


def parse_input(value: str) -> tuple[str, Path]:
    if "=" not in value:
        path = Path(value)
        return path.stem, path
    label, raw_path = value.split("=", 1)
    return label, Path(raw_path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("logs", nargs="+", metavar="[LABEL=]LOG")
    parser.add_argument("--output", type=Path, help="write CSV to this path")
    args = parser.parse_args()

    try:
        rows = [parse_log(label, path) for label, path in map(parse_input, args.logs)]
    except (OSError, ValueError) as error:
        parser.error(str(error))

    fieldnames = list(rows[0])
    output = args.output.open("w", newline="") if args.output else sys.stdout
    try:
        writer = csv.DictWriter(output, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    finally:
        if args.output:
            output.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
