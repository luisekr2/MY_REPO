# ROB compression SC retained-shadow depth experiment

## Objective

The SC-shadow leader avoids single-port SRAM training conflicts when prediction
metadata is still exact or can be repaired from a retained write-buffer row.
This experiment doubles only the SC write-buffer depth from four to eight using
the opt-in `RobCompressOnScShadowWriteBuffer8Config`.  ROB grouping, rename
recovery, TAGE training, and predictor SRAM geometry remain unchanged.

Entries remain valid after draining to SRAM, so a larger buffer retains more
recent coherent rows.  The intended effect is to convert
`read_free_stale_fallback` events into exact shadow repairs without adding an
SRAM read port.

## Validation and cost

Scala compilation, formatting, RTL elaboration, and the Verilator emulator
build completed successfully.  The config banner confirms an SC write-buffer
depth of eight with the leader's ROB-compression parameters.  All 26 generated
SRAM files are byte-identical to the leader.  `SimTop.sv` grows from 1,962,391
to 1,967,432 bytes, about 5.0 KiB or 0.26%; the individual `Sc` RTL files are
unchanged after module factoring.

The bzip2-derived input SHA-256 is
`e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`.
Baseline and candidate use seed 1, the same NEMU reference, and an exact
50,000-cycle limit.

## 50K gate

| Metric | SC-shadow leader | Shadow-8 | Change |
|---|---:|---:|---:|
| Retired instructions | 110,652 | 109,823 | -829 (-0.75%) |
| IPC | 2.213040 | 2.196460 | -0.75% |
| Branch mispredicts | 1,248 | 1,246 | -2 |
| SC conflict candidates | 7,335 | 7,371 | +36 |
| SC snapshot trains | 1,206 | 1,167 | -39 |
| SC shadow repairs | 587 | 561 | -26 |
| SC identity fallback | 1,180 | 1,278 | +98 |
| SC stale fallback | 541 | 404 | -137 (-25.3%) |
| SC pending forward | 327 | 320 | -7 |
| Total BPU train stall | 3,968 | 3,905 | -63 |
| TAGE read conflict | 3,427 | 3,433 | +6 |
| Stale commit drops | 0 | 0 | 0 |

Both runs reached the explicit limit without an assertion, abort, critical
error, or difftest failure.  The mechanism does reduce stale fallback, but it
does not convert those events into more useful snapshot training.  Replacement
phase and predictor trajectory instead reduce both repairs and accepted
snapshot trains.  The small train-stall reduction does not compensate for 829
fewer retired instructions.

## Decision

Reject the larger retained shadow and do not run full bzip2.  Raw retained-row
capacity is not the limiting factor; exact identity coverage and the unchanged
TAGE single-port conflicts dominate.  Increasing the depth again would add
state while amplifying the same replacement-phase risk.

The next experiment treats ROB compression as a capacity budget rather than a
reason to perturb grouping.  The measured leader averages only 49.93 occupied
physical ROB entries out of 352 and has zero ROB-full cycles.  A smaller ROB can
therefore fund more long-history predictor entries while keeping grouping
byte-for-byte unchanged.
