# Conflict-triggered fresh-only SC read-free experiment

## Fixed comparison

- Benchmark: `benchmarks/bzip2-derived/bzip2-derived.bin`
- Reference: NEMU `workloads/bin/riscv64-nemu-interpreter-so`
- Arguments: `-b 0 -e 0 --seed 1 --force-dump-result`
- TAGE-only reference: 526,259 instructions, 245,738 cycles, IPC 2.141545
- OFF reference: 526,256 instructions, 248,004 cycles, IPC 2.121966
- Pass gate: GOOD TRAP, NEMU difftest, SC read-free activity, IPC > 2.141545

## Implemented policy

SC training uses the prediction snapshot only when all conditions hold:

1. prediction and SC training request the same bank in the same cycle;
2. prediction and training bank/set identities match for every enabled SC table;
3. all eight SC row mutation versions still match;
4. a commit-stage version recheck also passes.

Non-conflicting training retains the normal SRAM read. A stale or identity
mismatch retains the original conflict/backpressure behavior. Snapshot use is
all-or-nothing across path, global, backward, IMLI, and bias tables, so no SC
sum mixes counter values from different epochs.

SC SRAM responses are overlaid with the retained write-buffer value before
prediction and normal training consume them. Row versions advance on accepted
logical table updates, including updates pending in the write buffer.

## Results

| Workload/configuration | Instructions | Cycles | IPC | Branch mispredicts | Result |
|---|---:|---:|---:|---:|---|
| bzip2 OFF | 526,256 | 248,004 | 2.121966 | 6,837 | GOOD TRAP |
| bzip2 TAGE-only read-free | 526,259 | 245,738 | 2.141545 | 7,339 | GOOD TRAP |
| **bzip2 TAGE + SC read-free** | **526,261** | **244,095** | **2.155968** | **7,246** | **GOOD TRAP / PASS** |
| heat-calc TAGE-only read-free | 62,526 | 15,724 | 3.976469 | 111 | GOOD TRAP |
| **heat-calc TAGE + SC read-free** | **62,526** | **15,397** | **4.060921** | **95** | **GOOD TRAP / PASS** |

On bzip2, SC read-free reduces 1,643 cycles and raises IPC by 0.673% relative
to the TAGE-only configuration. It raises IPC by 1.602% relative to OFF. The
dynamic instruction delta from TAGE-only is 2 instructions (0.00038%).

On heat-calc, it reduces 327 cycles and raises IPC by 2.124% relative to the
TAGE-only configuration.

## SC activity

| Counter | bzip2-derived | heat-calc |
|---|---:|---:|
| Conflict candidates | 51,013 | 1,004 |
| Read-free snapshot trains | 3,122 | 183 |
| Identity fallback | 0 | 0 |
| Stale fallback at admission | 0 | 0 |
| Commit-stage stale drops | 304 | 3 |
| SC correct / TAGE wrong | 1,617 | 225 |
| SC wrong / TAGE correct | 555 | 4 |
| ROB entries saved | 343,568 | 20,970 |

The commit recheck prevented every observed late-stale snapshot from writing.
However, 304 bzip2 training operations were conservatively dropped after
admission. This is correctness-safe and performance-positive in this run, but
a production implementation should reserve the row or replay those updates
instead of discarding them.

## Prototype hardware cost

- Counter snapshot: 528 bits per carried SC metadata record
  - seven regular tables: `7 × 8 ways × 6 bits = 336 bits`
  - bias table: `32 ways × 6 bits = 192 bits`
- Snapshot versions and bank identity: `8 × 12 + 2 = 98 bits`
- Total new carried SC snapshot payload: 626 bits before aggregate alignment
  and container overhead.
- Row-version state: `8 tables × 2 banks × 128 rows × 12 bits = 24,576 bits`.
- Existing four-entry-per-bank SC write buffers are reused; the prototype adds
  lookup/overlay logic but no extra write-buffer entries.

This full snapshot is a functional/performance upper-bound prototype, not the
recommended final area implementation. Selected-counter snapshots and compact
epochs should be evaluated after preserving the measured behavior.

## Build resource controls

- Scala compile JVM limit: 12 GiB.
- RTL generation JVM limit: 24 GiB; peak RSS 15,956,828 KiB; swap 0.
- Verilator output split: 5000; generation at `-j1`; peak RSS 31,030,764 KiB;
  swap 0.
- 101 generated C++ files larger than 1 MiB compiled at `-j1`; peak RSS
  32,000,780 KiB; swap 0.
- Remaining generated files and link used `-j2`; peak RSS 4,131,788 KiB;
  swap 0.

## Evidence

- `results/stale-safe-readfree/tage-sc-readfree/bzip2.log`
- `results/stale-safe-readfree/tage-sc-readfree/bzip2.time`
- `results/stale-safe-readfree/tage-sc-readfree/heat-native.log`
- `results/stale-safe-readfree/tage-sc-readfree/heat-native.time`

## Verdict

The experimental gate is satisfied. With the same bzip2-derived workload and
NEMU difftest, conflict-triggered fresh-only SC read-free improves IPC from the
TAGE-only 2.141545 to 2.155968 while actively bypassing 3,122 SC training reads.
