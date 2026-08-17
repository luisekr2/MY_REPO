# 401.bzip2 representative RTL results

The binary executes the bzip2 block-sorting compression kernel on a
deterministic 1 KiB payload. NEMU completes at 526,255 instructions. All five
RTL runs completed with a good trap, no bad trap, no critical error, and NEMU
differential checking enabled. The 0--5 instruction spread is the simulator's
trap/drain boundary; the benchmark result and architectural trace agree.

| Configuration | Cycles | IPC | Speedup vs OFF | Branch mispredicts | TAGE read conflicts |
|---|---:|---:|---:|---:|---:|
| OFF | 248,004 | 2.121966 | baseline | 6,168 | 22,099 |
| ROB compression ON | 240,263 | 2.190329 | +3.222% | 6,168 | 24,342 |
| Branch isolated | 238,145 | 2.209830 | **+4.140%** | 6,089 | 23,233 |
| Stale-safe TAGE | 245,738 | 2.141545 | +0.922% | 6,639 | 4,543 |
| Stale-safe TAGE + SC shadow2 | 241,750 | 2.176869 | +2.587% | 6,368 | 4,672 |

## Interpretation

ROB compression is useful for this workload, but branch isolation is the best
of the measured configurations. It improves OFF by 4.14% and plain compression
by 0.89%, while reducing mispredictions by 79 versus OFF. Plain compression
increases TAGE read conflicts by 10.15% (22,099 to 24,342), consistent with the
original conflict hypothesis.

The stale-safe TAGE mechanisms remove roughly 80% of recorded TAGE conflicts,
but do not recover all throughput here. TAGE-only increases committed
mispredictions to 6,639; adding SC shadow repair recovers part of that loss but
still trails branch isolation by 1.49%. SC shadow2 observed 43,732 candidates
and performed 3,930 repairs.

The compressed-instruction counter is 668,233 for plain ON and about 527K for
the branch-isolated variants. These are dynamic compression events rather than
unique committed ISA instructions, so they may exceed the instruction count.
