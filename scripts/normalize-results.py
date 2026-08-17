#!/usr/bin/env python3
"""Normalize XiangShan perf logs and calculate OFF relative to ON."""

from pathlib import Path
import csv
import re
import statistics

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "results" / "raw"
METRICS = {
    "cycles": (r"\b(?:cpu_cycle|clock_cycle)\b\D+(\d+)",),
    "instructions": (r"\bcommitInstr\b\D+(\d+)",),
    "rob_allocations": (r"\brob_allocated_entries\b\D+(\d+)",),
    "compressed_instructions": (r"\brob_compressed_instr\b\D+(\d+)",),
    "rob_entries_saved": (r"\brob_entries_saved\b\D+(\d+)",),
    "rob_full_stalls": (r"\bstall_cycle_rob\b\D+(\d+)",),
}

def last_match(text, patterns):
    found = []
    for pattern in patterns:
        found.extend(int(x) for x in re.findall(pattern, text, re.MULTILINE))
    return found[-1] if found else None

rows = []
for log in sorted(RAW.glob("*/*/run-*.log")):
    text = log.read_text(errors="replace")
    row = {"variant": log.parents[1].name, "workload": log.parent.name,
           "run": int(log.stem.split("-")[-1])}
    row.update({key: last_match(text, pats) for key, pats in METRICS.items()})
    c, i = row["cycles"], row["instructions"]
    row["ipc"] = i / c if c and i is not None else None
    inp = (row["rob_allocations"] or 0) + (row["rob_entries_saved"] or 0)
    row["compression_ratio"] = (row["rob_entries_saved"] / inp) if inp else 0.0
    rows.append(row)

fields = ["variant", "workload", "run", *METRICS, "ipc", "compression_ratio"]
with (ROOT / "results" / "runs.csv").open("w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=fields)
    writer.writeheader(); writer.writerows(rows)

grouped = {}
for row in rows:
    grouped.setdefault((row["workload"], row["variant"]), []).append(row)
compare_fields = [*METRICS, "ipc", "compression_ratio"]
with (ROOT / "results" / "comparison.csv").open("w", newline="") as f:
    fields2 = ["workload", "metric", "on_mean", "off_mean", "off_vs_on_percent"]
    writer = csv.DictWriter(f, fieldnames=fields2); writer.writeheader()
    for workload in sorted({r["workload"] for r in rows}):
        for metric in compare_fields:
            vals = {}
            for variant in ("on", "off"):
                vals[variant] = [r[metric] for r in grouped.get((workload, variant), []) if r[metric] is not None]
            on = statistics.mean(vals["on"]) if vals["on"] else None
            off = statistics.mean(vals["off"]) if vals["off"] else None
            delta = ((off / on) - 1) * 100 if on not in (None, 0) and off is not None else None
            writer.writerow({"workload": workload, "metric": metric,
                             "on_mean": on, "off_mean": off, "off_vs_on_percent": delta})
print(f"normalized {len(rows)} runs")

