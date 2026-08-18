# ROB compression rename snapshot capacity experiment

## Objective

The instruction-distance snapshot experiment showed that moving checkpoint
placement without adding storage reduced useful recovery coverage.  This
follow-up keeps the SC-shadow ROB-compression leader unchanged except for
doubling the rename snapshot queue from four to eight entries through the
opt-in `RobCompressOnScShadowSnapshot8Config`.

The production default remains four snapshots.  Placement, physical-ROB-index
spacing, ROB grouping, branch prediction, and SC read-free training are
otherwise byte-for-byte equivalent to the leader.

## Validation

Scala compilation, formatting, RTL elaboration, and the Verilator emulator
build completed successfully.  The SC SRAM descriptions are unchanged.  The
representative generated `CtrlBlock` RTL grew by about 34 KiB and `Rename` by
about 1.9 KiB.

Both complete smoke workloads reached GOOD TRAP under NEMU difftest without an
assertion, abort, or critical-error marker:

| Workload | Candidate instructions | Candidate cycles | Candidate IPC | Mispredicts | Comparison |
|---|---:|---:|---:|---:|---|
| heat-native | 62,526 | 15,762 | 3.966882 | 107 | exact final baseline match |
| heat-stress-32 | 126,151 | 26,020 | 4.848232 | 94 | exact final baseline match |

## Short gate

At the 10,000-cycle bzip2 gate, the larger queue appeared promising:

| Metric | SC-shadow leader | Snapshot-8 | Change |
|---|---:|---:|---:|
| Retired instructions | 10,313 | 10,349 | +36 (+0.349%) |
| IPC | 1.031300 | 1.034900 | +0.349% |
| Branch mispredicts | 97 | 97 | 0 |
| Redirects using snapshot | 46 | 49 | +3 |
| Rename recovery stall | 510 | 483 | -27 (-5.29%) |
| ROB walk cycles | 511 | 483 | -28 |

Snapshot-8 generated 247 checkpoints and encountered no full candidate during
this interval.  This result justified a full run, but did not predict it.

## Full bzip2 result

The full deterministic workload reverses the short-gate result:

| Metric | SC-shadow leader | Snapshot-8 | Change |
|---|---:|---:|---:|
| Retired instructions | 526,261 | 526,258 | -3 |
| Cycles | 232,470 | 236,627 | +4,157 (+1.79%) |
| IPC | 2.263780 | 2.223998 | -1.76% |
| Branch mispredicts | 6,526 | 6,790 | +264 |
| Redirects using snapshot | 1,617 | 1,711 | +94 |
| Rename recovery stall | 24,736 | 25,341 | +605 |
| Control recovery stall | 24,548 | 25,221 | +673 |
| ROB walk instructions | 67,352 | 67,666 | +314 |
| Wait for RAB walk end | 4,444 | 4,575 | +131 |

Both configurations reached GOOD TRAP.  Neither saw ROB enqueue pressure, FTQ
commit-queue pressure, stale SC commit drops, or control-flow sharing inside a
compressed ROB entry.  The candidate generated 13,231 checkpoints and still
had no full candidate, so capacity backpressure is not the explanation.

The changed snapshot availability alters redirect timing and consequently the
long-run branch-predictor trajectory.  That trajectory produces 264 extra
mispredictions and overwhelms the local recovery benefit seen in the first
10,000 cycles.  The two smoke workloads being exact final matches does not
offset the bzip2 regression; their prediction traces are too small to expose
the phase effect.

## Decision

Reject eight rename snapshots.  Raw ROB snapshot capacity is not a performance
bottleneck on the measured workloads, and a 10,000-cycle gate is insufficient
for policies that perturb recovery timing.  Future predictor/recovery
experiments must pass at least a 50,000-cycle bzip2 gate before a full run.

The next candidate keeps ROB grouping and rename recovery unchanged and instead
extends the already-coherent SC write-buffer shadow.  This targets exact recent
row reuse without consuming an SRAM read port and without changing architectural
recovery timing directly.
