# SC shadow-only + AUIPC-only ROB compression

## Decision

Rejected.  The candidate passed NEMU difftest and preserved all explicit
branch-isolation/stale-safety invariants, but it was slower than the SC
shadow-only leader on the fixed bzip2-derived workload.

| Configuration | Cycles | IPC | Branch mispredicts | ROB entries saved |
|---|---:|---:|---:|---:|
| SC shadow-only leader | 232,470 | 2.263780 | 6,526 | 335,397 |
| Direct-JAL-only candidate | 234,078 | 2.248212 | not promoted | not promoted |
| AUIPC-only candidate | 236,164 | 2.228367 | 6,699 | 335,995 |

Relative to the leader, AUIPC-only is 1.589% slower by cycles and loses
1.564% IPC.  It shares only 117 control-flow-class instructions and saves 598
additional ROB entries, but incurs 173 additional mispredictions.  Even a
non-redirecting AUIPC changes rename/retirement packing and therefore the
timing/order of later predictor training enough to perturb the trajectory.

## Safety counters

- GOOD TRAP / NEMU difftest pass
- `rob_conditional_branch_shared_entry = 0`
- `rob_control_flow_shared_entry = 117` (AUIPC only)
- `read_free_commit_stale_drop = 0`
- TAGE `read_conflict = 21,966`
- frontend `train_stall = 24,831`
- SC identity/stale/pending fallback = 7,106 / 2,997 / 7,466

Raw output is in `bzip2-derived/run.log`; host resource timing is in
`bzip2-derived/run.time`.  Candidate source commit: `8aa5454`.

## Consequence

Do not recover ROB compression across JAL, JALR, conditional branches, or
AUIPC in the promoted configuration.  Further optimization should keep the
SC shadow-only ROB grouping unchanged and target SC fallback coverage instead.
