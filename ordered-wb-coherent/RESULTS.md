# Coherent ordered TAGE write-buffer forwarding

## Purpose

This experiment tests whether the extra mispredictions seen with ordered
entry forwarding were caused by a mixed logical TAGE state: a forwarded
entry/taken counter paired with an older useful counter read from SRAM.

The implementation forwards both the entry and useful counter from their
bank-local pending-write buffers.  A useful-counter reset has priority.  It
does not add an SRAM port and all tests use seed 1 with NEMU difftest.

## Fast-workload results

| Workload | Branch-isolated IPC | Entry-only IPC | Coherent IPC | Branch-isolated mispredicts | Coherent mispredicts |
| --- | ---: | ---: | ---: | ---: | ---: |
| heat-native | 3.910808 | 3.955089 | 3.955089 | 124 | 113 |
| heat-stress | 4.865998 | 4.865998 | 4.865998 | 89 | 89 |
| bzip2-derived | 2.209830 | 2.204601 | 2.204601 | 6,812 | 6,859 |

All three binaries reached GOOD TRAP and exited successfully.  Every reported
cycle count, IPC, misprediction count, TAGE read-conflict count, and training
stall count is identical to the entry-only experiment:

| Workload | TAGE read conflicts | Training stalls |
| --- | ---: | ---: |
| heat-native | 1,205 | 3,949 |
| heat-stress | 293 | 688 |
| bzip2-derived | 21,682 | 46,446 |

## Decision

The mixed entry/useful state is eliminated structurally, but it was not the
cause of the bzip2 regression.  Pending-entry forwarding itself changes when
trained state becomes visible to prediction.  This helps the regular native
heat loop but changes the predictor trajectory unfavourably on bzip2; the
previous MCF entry-only result showed the same direction.

Consequently this forwarding design is retained as a diagnostic experiment,
not as the default.  The default remains branch-isolated ROB compression,
which preserves the baseline prediction-state visibility.  Future work is
restricted to removing additional prediction/training pressure caused by ROB
compression; it must not redefine TAGE prediction semantics merely to improve
an individual workload.

An MCF rerun was intentionally skipped: the coherent useful-counter path was
observationally inactive in all three discriminating fast tests, which were
bit-identical to entry-only, while an entry-only MCF run already took about
one hour and regressed from 1.678362 to 1.669070 IPC.

## Artifacts

- Source repository: `rob-compress-ordered-wb-forward/`
- Emulator: `rob-compress-ordered-wb-forward/build-ordered-wb/verilator-compile/emu`
- Build log: `results/build/ordered-wb-coherent-emu.log`
- Raw fast-workload logs and resource reports: this directory
