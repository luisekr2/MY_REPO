# ROB compression and read-free TAGE training: A/B/C/D smoke test

All benchmark runs use the same binary, seed 1, NEMU difftest, and forced
performance-counter dump. The new C runs terminate with exit status 0 and GOOD
TRAP.

| ID | ROB compression | Branch isolation | Read-free training |
|---|---|---|---|
| A | OFF | n/a | OFF |
| B | ON | ON | OFF |
| C | OFF | n/a | ON |
| D | ON | ON | ON |

## End-to-end performance

| Benchmark | ID | Cycles | IPC | Change from A |
|---|---|---:|---:|---:|
| native | A | 16,024 | 3.902022 | baseline |
| native | B | 15,988 | 3.910808 | -36 (-0.225%) |
| native | C | 15,812 | 3.954338 | -212 (-1.323%) |
| native | D | 15,578 | 4.013737 | -446 (-2.783%) |
| TAGE-stress | A | 25,999 | 4.852148 | baseline |
| TAGE-stress | B | 25,925 | 4.865998 | -74 (-0.285%) |
| TAGE-stress | C | 25,951 | 4.861123 | -48 (-0.185%) |
| TAGE-stress | D | 25,936 | 4.863934 | -63 (-0.242%) |

## Factor and interaction effects

Negative cycle deltas are improvements.

| Benchmark | Comparison | Meaning | Cycle delta |
|---|---|---|---:|
| native | B - A | compression/isolation without read-free | -36 |
| native | C - A | read-free without compression | -212 |
| native | D - C | compression/isolation with read-free | -234 |
| native | D - B | read-free with compression | -410 |
| native | (D-B) - (C-A) | interaction | -198 |
| TAGE-stress | B - A | compression/isolation without read-free | -74 |
| TAGE-stress | C - A | read-free without compression | -48 |
| TAGE-stress | D - C | compression/isolation with read-free | -15 |
| TAGE-stress | D - B | read-free with compression | +11 |
| TAGE-stress | (D-B) - (C-A) | interaction | +59 |

The native layout shows a positive interaction: read-free saves 212 cycles
without compression but 410 cycles with compression. The additional 198-cycle
benefit is consistent with compression changing training timing in a way that
amplifies the existing conflict. TAGE-stress does not reproduce this behavior;
its interaction is adverse by 59 cycles and all end-to-end deltas are small.

## TAGE and ROB counters

| Benchmark | ID | Train | Read-free train | Read conflict | Commit mispredict | TAGE conditional mispredict | Allocation | ROB entries saved |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| native | A | 5,671 | n/a | 898 | 115 | 62 | 45 | 0 |
| native | B | 4,999 | n/a | 1,351 | 126 | 67 | 41 | 21,076 |
| native | C | 5,972 | 474 | 0 | 109 | 61 | 57 | 0 |
| native | D | 5,794 | 470 | 0 | 107 | 58 | 53 | 20,937 |
| TAGE-stress | A | 6,593 | n/a | 319 | 88 | 49 | 42 | 0 |
| TAGE-stress | B | 6,597 | n/a | 306 | 86 | 49 | 42 | 68,882 |
| TAGE-stress | C | 6,595 | 409 | 0 | 86 | 48 | 44 | 0 |
| TAGE-stress | D | 6,591 | 408 | 0 | 88 | 49 | 44 | 68,949 |

## Interpretation limits

The result establishes that read-free training has an independent effect in C:
it removes TAGE read conflicts even when ROB compression is disabled. It also
shows a workload-dependent interaction with compression.

This remains a single-seed smoke test. The full-snapshot prototype adds about
304 bits to BPU resolve metadata and lacks freshness/version validation, so it
can change provider and allocation decisions. The allocation counts differ
from A/B, confirming semantic divergence. In addition, historical A was built
from the original OFF worktree while B/C/D use the branch-isolated experiment
lineage; the source changes were intended to be controlled, but a publication
quality factorial test should rebuild A and B from this same source revision
with only the two feature switches changed.
