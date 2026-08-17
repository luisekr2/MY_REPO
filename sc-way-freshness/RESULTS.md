# SC exact one-step way-freshness experiments

## Objective

Raise performance above SC shadow-only without changing ROB grouping or adding
an SRAM port.  A stale row snapshot can be exact for the target counter if
exactly one intervening update changed only other ways.

## Correctness rule tested

Read-free training was permitted only when all of the following held:

1. prediction/training bank and row identity matched;
2. `snapshotVersion + 1 == currentVersion`;
3. the exact intervening update was present in proof state;
4. `interveningWayMask & trainingTargetWayMask == 0`;
5. the ordinary t2 version check still passed before commit.

This is a proof-based rule rather than a stale-age heuristic.

## Implementations and results

| Candidate | Proof state | Build | heat-stress host result | Decision |
|---|---|---|---|---|
| Per-row last-way mask | ~22.5 Kbit registers | RTL + emu pass | >30 min; aborted | Reject |
| Bank-local journal depth 4 | ~2 Kbit registers | RTL + emu pass | >30 min; aborted | Reject |
| Bank-local journal depth 1 | a few hundred bits | RTL + emu pass | >12 min; aborted | Reject |
| SC shadow-only reference | retained WB shadow | pass | 4:31.58 | Keep |

The same fixed `heat-calc-tage-stress.bin`, seed 1, and NEMU reference were
used.  Candidate runs produced no early assertion or BAD TRAP, but were stopped
after exceeding the host-runtime acceptance bound by a large margin.  The
wide target-mask/freshness proof logic lies in SC training's combinational
evaluation cone; reducing stored state did not restore emulator throughput.
This also flags synthesis timing risk even though SRAM ports are unchanged.

## Artifacts

- Per-row source commit: `3bcc36f`
- Depth-4 journal commit: `a83daf0`
- Depth-1 journal commit: `43d83f6`
- Source restoration commits: `42eb2cd`, `c4e7fa5`, `82fb63b`
- Build directories: `build-wayfresh`, `build-wayjournal`, `build-waydepth1`
- Partial run logs and time files: `results/sc-wayfresh`,
  `results/sc-wayjournal`, `results/sc-waydepth1`

The promoted working source is restored to the exact SC shadow-only identity
and freshness behavior.  These candidates must not be used as the final
configuration.
