# Stale-safe read-free TAGE experiment

## Configuration

- Repository: `rob-compress-readfree-training`
- Branch: `experiment/tage-readfree-training`
- XiangShan config: `RobCompressOffReadFreeTrainingConfig`
- ROB compression: disabled, so this experiment isolates the BPU change.
- Simulator: Verilator, seed 1, NEMU difftest enabled.
- Memory-conscious build: Verilator output split 5000, large generated C++ files
  compiled sequentially, remaining files at low parallelism.

The prototype adds retained entry/useful-counter forwarding, hashed mutation
versions, one pending critical stale repair per TAGE table, and one pending
natural-read allocation intent per table.

## Correctness

All three new runs completed with GOOD TRAP and passed NEMU difftest:

- heat-calc native
- heat-calc TAGE stress
- bzip2-derived (SPECint-like)

## Performance

### heat-calc

| Workload/configuration | Instructions | Cycles | IPC | Branch mispredicts |
|---|---:|---:|---:|---:|
| Native, prior OFF + read-free | 62,526 | 15,812 | 3.954338 | 107 |
| Native, stale-safe | 62,526 | 15,804 | 3.956340 | 104 |
| TAGE stress, prior OFF + read-free | 126,151 | 25,951 | 4.861123 | 86 |
| TAGE stress, stale-safe | 126,151 | 25,951 | 4.861123 | 86 |

Native improves by 8 cycles (0.051%) over the prior read-free result. TAGE
stress is exactly unchanged. These are correctness-positive but not meaningful
speedups.

### bzip2-derived (SPECint-like)

| Configuration | Instructions | Cycles | IPC | Branch mispredicts | TAGE read conflicts |
|---|---:|---:|---:|---:|---:|
| OFF baseline | 526,256 | 248,004 | 2.121966 | 6,837 | 22,099 |
| Prior read-free result | 526,255 | 252,385 | 2.085128 | 7,617 | 0 |
| Stale-safe read-free | 526,256 | 256,247 | 2.053706 | 7,337 | 0 |

Relative to OFF, stale-safe read-free takes 3.324% more cycles and has 3.217%
lower IPC. Relative to the prior read-free result, it takes 1.530% more cycles
and has 1.507% lower IPC. Mispredictions improve by 3.676% relative to prior
read-free, but remain 7.313% above OFF.

The prior read-free row came from the existing bzip2-derived report and was
labelled branch-isolated + read-free there. The current run explicitly reports
ROB compression disabled, so the OFF comparison is the cleanest direct
reference.

## Stale-safe activity

| Workload | Snapshot trains | Stale provider writes blocked | Repair enqueued | Repair applied | Repair dropped | Allocation intent applied |
|---|---:|---:|---:|---:|---:|---:|
| heat-calc native | 637 | 1,249 | 19 | 19 | 1 | 0 |
| heat-calc TAGE stress | 606 | 1,183 | 8 | 8 | 0 | 0 |
| bzip2-derived | 20,556 | 4,596 | 316 | 312 | 342 | 0 |

The bzip2-derived run demonstrates that the stale detector and natural-read
repair path are exercised. However, only 312 deferred repairs are applied,
while 4,596 stale provider writes are blocked. The one-entry-per-table repair
slot also experiences replacement/drop pressure. Allocation intent never fires
in these workloads.

## Interpretation

This prototype is stale-safe in the tested paths and eliminates TAGE training
read-bank conflicts, but it is not a performance win on the SPECint-like test.
The result suggests:

1. Removing the training read conflict is insufficient when prediction quality
   is still below OFF.
2. Blocking all stale provider writes is conservative; most are not repaired
   before their information is lost.
3. A one-entry repair slot is too shallow for bzip2-like update pressure.
4. Repairing only on a later natural read adds variable delay, so frequently
   useful branch state can remain stale.
5. The allocation-intent mechanism currently adds logic but contributes no
   measured benefit in these tests.

## Hardware-cost caveat

The prototype uses 12,288 bits of hashed version state
(8 tables × 4 banks × 2 ways × 16 buckets × 12 bits), plus 192 version bits in
each carried TAGE metadata snapshot and one repair/allocation record per table.
The older entry/useful snapshots are still retained, so this is not yet the
minimum-area implementation.

## Recommendation

Do not treat the current design as the final implementation. The next useful
experiment is a less conservative coherent-overlay scheme:

- forward every accepted logical update to prediction immediately;
- use the version only to reject a repair when its tag/identity is invalid;
- coalesce stale updates by exact `{table, bank, set, way, tag}` identity;
- remove the unused allocation-intent path unless a targeted benchmark proves
  that it is exercised;
- measure repair age, coalescing rate, slot occupancy, and unrepaired stale
  events before increasing queue depth.

This targets the observed failure mode without blindly adding a large repair
queue.
