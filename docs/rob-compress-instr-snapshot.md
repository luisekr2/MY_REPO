# ROB compression-aware rename snapshot experiment

## Objective

ROB compression stores several architectural instructions in one physical ROB
entry.  Rename snapshot spacing, however, was measured as a physical ROB-index
distance of `RobCommitWidth * 4` entries.  The experiment tests whether that
makes snapshots too sparse in architectural-instruction space and increases
rename recovery latency.

The production default remains unchanged.  The opt-in
`RobCompressOnScShadowInstrSnapshotConfig` replaces only the spacing source:

- baseline: distance from the last snapshot's physical ROB index;
- candidate: saturating count of renamed architectural instructions;
- threshold: 32 instructions (`RobCommitWidth * 4`);
- reset: a successfully generated snapshot or redirect;
- placement and capacity: unchanged jump-only placement and four snapshots.

The counter uses `lastUop`, so a multi-uop architectural instruction is counted
once.  Three counters expose the mechanism:

- `rename_snapshot_create`;
- `rename_snapshot_full_candidate`;
- `rename_snapshot_instr_distance_reached_cycles`.

The common summary script also extracts snapshot use, redirect, rename recovery,
ROB walk, and ROB/FTQ pressure counters.

## Validation

Scala compilation, formatting, generated RTL, and the Verilator emulator build
completed successfully.  Generated RTL contains a six-bit saturating counter
with a value of 32 and reset paths for redirects and successful snapshots.  The
SC SRAM descriptions are unchanged from the leader.

Two complete smoke workloads reproduce the instrumented leader exactly:

| Workload | Candidate instructions | Candidate cycles | Candidate IPC | Mispredicts | Snapshot use | Recovery stall | Comparison |
|---|---:|---:|---:|---:|---:|---:|---|
| heat-native | 62,526 | 15,762 | 3.966882 | 107 | 13 | 1,074 | exact baseline match |
| heat-stress-32 | 126,151 | 26,020 | 4.848232 | 94 | 32 | 726 | exact baseline match |

Both runs reached GOOD TRAP under NEMU difftest without an assertion, abort, or
critical-error marker.

## bzip2 gate

The performance gate uses the same image, seed 1, NEMU reference, and an exact
10,000-cycle limit for both configurations.

| Metric | SC-shadow leader | Instruction-distance candidate | Change |
|---|---:|---:|---:|
| Retired instructions | 10,313 | 10,257 | -56 (-0.543%) |
| IPC | 1.031300 | 1.025700 | -0.543% |
| Branch mispredicts | 97 | 97 | 0 |
| Redirects | 101 | 101 | 0 |
| Redirects using snapshot | 46 | 44 | -2 |
| Rename recovery stall | 510 | 537 | +27 (+5.29%) |
| ROB walk cycles | 511 | 537 | +26 |
| Wait for RAB walk end | 91 | 93 | +2 |
| ROB enqueue disallowed cycles | 0 | 0 | 0 |
| FTQ commit queue full cycles | 0 | 0 | 0 |

The candidate generated 181 successful snapshot events, encountered 13
snapshot-full candidates, and held the instruction-distance threshold reached
for 2,704 cycles.  Therefore the new distance condition is active, but it does
not improve useful recovery coverage.  The changed checkpoint phase instead
reduces snapshot-backed redirects and increases recovery walking.

An initial version suppressed a candidate when the snapshot queue was full.
The final version restores the leader's original queue-full semantics so the
only policy change is the distance source.  Both versions produced identical
bzip2 10,000-cycle architectural and performance results; the extra condition
was not responsible for the regression.

## Decision

Do not promote instruction-count snapshot spacing and do not spend time on the
full 232k-cycle bzip2 run.  It fails the short gate with lower IPC and materially
higher recovery stall while leaving branch-prediction and ROB-capacity pressure
unchanged.

The result points to snapshot selection and replacement, not raw spacing, as the
more promising ROB-linked direction.  A future experiment should rank snapshot
candidates by expected recovery value (for example, unresolved control-flow
coverage and distance to the oldest unresolved branch) or add a lightweight
branch checkpoint tier.  Simply creating checkpoints sooner competes for the
same four entries and can evict or phase-shift more useful checkpoints.

Input SHA-256 values:

- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
