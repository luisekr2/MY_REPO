# heat-calc ROB compression and TAGE smoke analysis

## Provenance

- Source inspiration: `toddmaustin/mojo-v`
- Upstream revision inspected: `4e57b76abafb0a46a204fd6476e4a1c81d8d238c`
- Upstream source: `bringup-bench/heat-calc/heat-calc.cc`
- Local benchmark: `/home/joe/work/01_GEN2_COMP/benchmarks/heat-calc-rob`
- Target: freestanding RV64GC
- Grid points: 100
- Smoke time steps: 32
- Compiler: Ubuntu Clang 21.1.8, `-O3`
- Seed: 1
- Difftest and forced performance dump: enabled

The native variant preserves the FP64 stencil and its 98/100-trip loops. The
TAGE-stress variant preserves those trip counts but replaces FP work with
independent ROB-compressible integer operations. Mojo-V encrypted types and
custom encrypted load/store instructions are excluded because the evaluated
XiangShan configurations implement RV64GC.

## Validation

All six runs:

- exited with status 0;
- reached GOOD TRAP;
- passed difftest;
- committed an identical instruction count within each benchmark;
- produced no branch-isolation assertion failure.

## Performance

| Benchmark | Configuration | Instructions | Cycles | IPC |
|---|---|---:|---:|---:|
| native | OFF | 62,526 | 16,024 | 3.902022 |
| native | ON | 62,526 | 16,048 | 3.896186 |
| native | branch-isolated | 62,526 | 15,988 | 3.910808 |
| TAGE-stress | OFF | 126,151 | 25,999 | 4.852148 |
| TAGE-stress | ON | 126,151 | 25,989 | 4.854015 |
| TAGE-stress | branch-isolated | 126,151 | 25,925 | 4.865998 |

Branch-isolated versus OFF:

- native: 36 fewer cycles (0.225%);
- TAGE-stress: 74 fewer cycles (0.285%).

Branch-isolated versus current ON:

- native: 60 fewer cycles (0.374%);
- TAGE-stress: 64 fewer cycles (0.246%).

These are single-seed smoke measurements and the deltas are small.

## ROB compression

| Benchmark | Configuration | ROB input | Allocated | Entries saved | Control-flow shared |
|---|---|---:|---:|---:|---:|
| native | OFF | 66,463 | 66,463 | 0 | n/a |
| native | ON | 66,971 | 38,735 | 28,236 | not instrumented |
| native | branch-isolated | 66,621 | 45,545 | 21,076 | 0 |
| TAGE-stress | OFF | 130,635 | 130,635 | 0 | n/a |
| TAGE-stress | ON | 130,521 | 55,219 | 75,302 | not instrumented |
| TAGE-stress | branch-isolated | 130,457 | 61,575 | 68,882 | 0 |

The stress binary successfully creates high ROB compression while branch
isolation retains most of the saved entries.

## TAGE

| Benchmark | Configuration | TAGE trains | Read-conflict cycles | Loop-conflict cycles | Commit mispredicts |
|---|---|---:|---:|---:|---:|
| native | OFF | 5,671 | 898 | 593 | 115 |
| native | ON | 5,213 | 1,101 | 826 | 128 |
| native | branch-isolated | 4,999 | 1,351 | 956 | 126 |
| TAGE-stress | OFF | 6,593 | 319 | 142 | 88 |
| TAGE-stress | ON | 6,590 | 299 | 123 | 91 |
| TAGE-stress | branch-isolated | 6,597 | 306 | 140 | 86 |

The native port exposes more TAGE conflicts under compression, but branch
isolation does not reduce that counter in this short run. The integer stress
variant does not amplify TAGE conflict. Therefore these binaries validate
performance and ROB behavior, but do not yet prove that branch isolation
improves performance by removing a TAGE bank conflict.

The next targeted revision should sweep hot-loop PC padding and branch slot
placement, selecting a layout where current ON increases TAGE conflict and
branch-isolated returns it toward OFF while preserving the same checksum and
committed instruction count.
