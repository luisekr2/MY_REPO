# ROB compression: control-flow back-compression refinement

## Motivation

The performance leader keeps conditional branches and jumps in independent
ROB entries.  This experiment asks whether some of that capacity can be
recovered without reintroducing the branch-prediction sensitivity that made
the fully compressed policy slower.

## All-jump attempt

The first opt-in policy allowed both direct JAL and indirect JALR instructions
to join the preceding compressible group while conditional branches remained
independent.

At the 10K bzip2 gate it shared 235 jump entries and increased saved ROB
entries from 3,285 to 3,507.  Performance nevertheless regressed:

| metric | leader | all-jump back-compression |
| --- | ---: | ---: |
| instructions at 10K cycles | 10,313 | 10,284 |
| IPC | 1.031300 | 1.028400 |
| resolve mispredicts | 97 | 99 |
| recovery stall cycles | 510 | 497 |

The local recovery cost fell, but indirect-jump/checkpoint and predictor timing
changed enough to lose 0.281% throughput.  The all-jump policy was rejected at
the short gate.

## Direct-JAL refinement

The follow-up kept conditional branches and JALR independent and permitted only
direct JAL to join the preceding group.  It passed the intermediate gates but
failed the complete workload:

| workload | leader cycles | direct-JAL cycles | IPC change | mispredicts (base / candidate) |
| --- | ---: | ---: | ---: | ---: |
| heat-native | 15,762 | 15,749 | +0.083% | 107 / 107 |
| heat-stress-32 | 26,020 | 26,067 | -0.180% | 94 / 98 |
| bzip2 50K cycles | fixed 50K | fixed 50K | +0.0425% | 1,248 / 1,252 |
| bzip2 complete | 232,470 | 234,078 | -0.688% | 6,526 / 6,674 |

The full candidate shared 15,855 direct-JAL entries and saved 15,764 extra ROB
entries, but added 148 resolve mispredicts and 1,608 cycles.  It reached GOOD
TRAP with no assertion, abort, or critical error.  Direct-JAL back-compression
was rejected because checkpoint and predictor timing still changed the long-run
trajectory.

## AUIPC-only refinement

AUIPC uses the jump execution type but does not redirect control flow.  The
next policy therefore keeps conditional branches, JAL, and JALR independent
and permits only AUIPC to join the preceding group.  `RobCompressAuipcBack`
defaults to false; the experiment config alone enables it.  Assertions require
every real control-flow instruction to own a one-instruction ROB entry, and a
dedicated counter exposes AUIPC sharing.

The AUIPC-only grouping candidate failed the 10K gate despite sharing only 34
AUIPC entries: instructions fell from 10,313 to 10,195 (-1.144%).  Inspection
showed that AUIPC inherits the jump execution type and is also considered for a
rename recovery snapshot.  Attaching AUIPC to the preceding group therefore
still shifts checkpoint placement.  The grouping candidate was removed.

## Skip AUIPC recovery snapshots

The next experiment restores byte-for-byte leader grouping and changes only
snapshot eligibility.  AUIPC does not redirect control flow, so an AUIPC
checkpoint cannot be the direct recovery point for a branch or jump.  The
opt-in `RenameSnapshotSkipAuipc` policy excludes AUIPC while retaining JAL and
JALR snapshots.  A counter reports cycles in which an AUIPC would otherwise
have been an eligible snapshot candidate.

The policy was active but failed the 50K gate. It removed 53 AUIPC snapshot
candidates and reduced resolve mispredicts from 1,248 to 1,241, yet retired
instructions fell from 110,652 to 110,132 (-0.470%). Recovery stall also fell
from 5,462 to 5,301, demonstrating that the local recovery metrics improved
while the global execution/memory phase worsened. The parameter and policy
were removed; logs remain under `results/rob-compress-skip-auipc-snapshot/`.
