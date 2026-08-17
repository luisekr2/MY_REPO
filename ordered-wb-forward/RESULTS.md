# Always-on ordered TAGE write-buffer forwarding

## Design

- Base: branch-isolated ROB compression.
- Reuse the existing four-entry, bank-local TAGE entry write buffers as the
  latest logical entry source for both prediction and training reads.
- No runtime enable/disable policy, prediction snapshot, mutation-version
  table, repair queue, or additional SRAM port.
- The generated TAGE entry and useful-counter SRAM macros are byte-identical
  to branch-isolated and retain one combined `RW0` port.

## Correctness

All four workloads completed with NEMU difftest enabled, reached GOOD TRAP,
and exited with status zero. No critical-error message was observed.

## Performance

| Workload | OFF IPC | Branch-isolated IPC | Ordered-WB IPC | vs OFF | vs branch-isolated |
| --- | ---: | ---: | ---: | ---: | ---: |
| heat-native | 3.902022 | 3.910808 | 3.955089 | +1.360% | +1.132% |
| heat-stress | 4.852148 | 4.865998 | 4.865998 | +0.285% | 0.000% |
| bzip2-derived | 2.121966 | 2.209830 | 2.204601 | +3.894% | -0.237% |
| mcf-connected-16-r2 | 1.662466 | 1.678362 | 1.669070 | +0.397% | -0.554% |

- Three-fast-workload geometric mean: +1.835% vs OFF and +0.297% vs
  branch-isolated.
- All-four-workload geometric mean: +1.474% vs OFF and +0.083% vs
  branch-isolated.

## Predictor diagnosis

| Workload | Branch-isolated mispredicts | Ordered-WB mispredicts | Delta |
| --- | ---: | ---: | ---: |
| heat-native | 124 | 113 | -11 |
| heat-stress | 89 | 89 | 0 |
| bzip2-derived | 6,812 | 6,859 | +47 |
| mcf-connected-16-r2 | 21,168 | 21,336 | +168 |

Forwarding pending entry counters helps the tight regular loop, is inactive or
neutral on heat-stress, and changes the TAGE state trajectory unfavorably on
the more alias-sensitive bzip2 and MCF workloads. The aggregate gain over
branch-isolated is only 0.083%, so this implementation is not a convincing new
default despite its low storage and SRAM-port cost.

## Artifacts

- RTL/source repository: `rob-compress-ordered-wb-forward/`
- Emulator: `rob-compress-ordered-wb-forward/build-ordered-wb/verilator-compile/emu`
- Fast logs: `results/ordered-wb-forward/`
- MCF log: `../specint2006-investigation/results/mcf-connected-16-r2/ordered-wb-forward/`
