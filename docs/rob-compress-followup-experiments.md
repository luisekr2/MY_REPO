# ROB compression follow-up experiments

## Scope and decision rule

These experiments try to improve the `RobCompressOnScShadowOnlyConfig` leader.
The fixed reference is the complete bzip2-derived run at 526,261 retired
instructions, 232,470 cycles, IPC 2.263780, 6,526 resolve-time branch
mispredictions, and 5,847 commit-time branch mispredictions. Every complete
candidate below reached GOOD TRAP under NEMU difftest without assertion, abort,
or critical error.

Short fixed-cycle gates are useful for rejecting bad ideas but are not
promotion evidence. Several policies improved 50K throughput and then changed
the long-run branch-predictor trajectory enough to lose performance. Only a
positive complete run with no material safety regression can replace the
leader.

## TAGE feedback decoupling

Three attempts reduced local predictor contention but failed the throughput
gate:

| Policy | 10K result | 50K result | Local effect | Decision |
| --- | ---: | ---: | --- | --- |
| Read-free TAGE only for conditional mispredict snapshots | exact | -0.515% | 50K read conflicts 3,427 -> 2,820; stalls 3,968 -> 3,482 | reject |
| One-entry training skid for every conflict | -0.281% | not run | 10K stalls 574 -> 443 | reject |
| Training skid only for multi-branch feedback | exact | -0.978% | 50K stalls 3,968 -> 3,771 | reject |

Avoiding a collision changes when later training becomes visible. Lower local
stall or conflict counters therefore do not imply better prediction accuracy.
The implementation code was removed; archived counters remain supported by the
summary script.

## Multiply-aware ROB group boundaries

Keeping a multiply attached to older instructions while ending its group at
the multiply reduced `waitMulCycle` without exposing the multiply as a new
group head. The unrestricted policy looked strong at short gates but failed the
complete workload:

| Metric | Leader | Break after every MUL | Change |
| --- | ---: | ---: | ---: |
| 10K IPC | 1.031300 | 1.033300 | +0.194% |
| 50K IPC | 2.213040 | 2.232380 | +0.874% |
| Complete cycles | 232,470 | 232,753 | +283 (+0.122%) |
| Complete IPC | 2.263780 | 2.260998 | -0.123% |
| Complete resolve mispredicts | 6,526 | 6,565 | +39 |
| Complete queue-full cycles | 4,192 | 4,923 | +731 |
| Complete MUL-wait cycles | 447 | 246 | -201 |

A tail-selective refinement made a boundary only when at least three younger
compressible instructions followed the multiply. It was inactive and exactly
matched the leader at 10K. At 50K, only 12 boundaries still produced +0.865%
IPC, one fewer resolve misprediction, and 25 fewer queue-full cycles. The
complete run nevertheless rejected it:

| Metric | Leader | MUL tail >= 3 | Change |
| --- | ---: | ---: | ---: |
| Retired instructions | 526,261 | 526,259 | -2 |
| Cycles | 232,470 | 234,270 | +1,800 (+0.774%) |
| IPC | 2.263780 | 2.246378 | -0.769% |
| Resolve / commit mispredicts | 6,526 / 5,847 | 6,645 / 5,919 | +119 / +72 |
| Effective boundaries | 0 | 18 | +18 |

Only 18 complete-run boundaries were enough to alter the long-run predictor
trajectory materially. Break-before-MUL and no-MUL-compression variants were
also rejected: making the multiply a group head increased or failed to improve
MUL wait instead of preserving the useful older-to-MUL overlap. All multiply
boundary code and parameters were removed.

## SC threshold and component tuning

The leader's SC corrector reports 1,775 cases where SC fixes TAGE and 582 where
SC overrides a correct TAGE prediction. Its adaptive threshold also attempts
to move below the default minimum 536 times. A candidate lowered only that
minimum from 832 to 704. It exactly matched every baseline counter through 50K,
so the change was isolated to the later saturated interval, but the complete
run regressed:

| Metric | Leader | Lower SC floor | Change |
| --- | ---: | ---: | ---: |
| Cycles | 232,470 | 234,918 | +2,448 (+1.053%) |
| IPC | 2.263780 | 2.240177 | -1.043% |
| Resolve / commit mispredicts | 6,526 / 5,847 | 6,651 / 5,961 | +125 / +114 |
| SC predictions used | 7,990 | 7,914 | -76 |
| Threshold underflow attempts | 536 | 10 | -526 |

The adaptive counter's preferred direction is not an accuracy oracle because
changing the floor changes the subsequent prediction and training trajectory.

A second candidate disabled the bias component after its debug counter showed
more wrong than correct directions. It failed the 10K gate immediately: IPC
fell from 1.031300 to 1.027900 (-0.330%) and resolve mispredictions rose from 97
to 99. Source inspection explained why the debug ratio was misleading: the
component correctness counters are sampled only on selected SC training cases,
which are deliberately biased toward wrong or low-confidence predictions.
The component and threshold defaults were restored.

## Ahead-BTB write retention

The complete leader run dropped 1,496 Ahead-BTB writes when its four-entry
buffers filled, with 1,476 drops concentrated in bank 2.  Doubling every bank
to eight entries was an isolated test: it left the ROB, SC, TAGE, table
indexing, and training order unchanged and only retained requests that the
`ValidIO` interface would otherwise discard.

The candidate eliminated every drop, including the 12 drops visible at 50K,
but did not improve execution.  The 10K, 50K, and complete runs reproduced the
leader's instruction count, cycle count, IPC, and resolve/commit mispredict
counters exactly.  The complete run changed Ahead-BTB hits/misses from
166,106/40,379 to 166,068/40,417, showing that the retained writes were either
redundant or mildly harmful.  The config was removed; aggregate Ahead-BTB bank
metrics remain in `scripts/rob_compress_summary.py`.

## TAGE associativity with the ROB-size confound removed

The earlier four-way TAGE experiment was coupled to a 208-entry ROB.  At 50K
it lost 0.846% IPC, but also incurred 1,861 ROB-full stall cycles that the
352-entry leader did not have.  A new isolated config restores the 352-entry
ROB and changes only each TAGE table from two to four ways while preserving its
set count and history hash.

This corrected experiment is the first follow-up to beat the leader at the
complete gate: bzip2-derived improves from 232,470 to 231,680 cycles and from
IPC 2.263780 to 2.271474 (+0.340%).  Resolve/commit mispredicts fall by 10/35.
Both heat workloads remain exact cycle-level matches, for a three-workload IPC
geometric mean of +0.113%.  Full methodology, cost, and limitations are in
`docs/rob-compress-tage4way-352.md`.

## Final outcome

Promote the opt-in `RobCompressOnScShadowTage4WayConfig` as the measured
performance leader while retaining `RobCompressOnScShadowOnlyConfig` as the
lower-cost reference.  Local latency, conflict, recovery, or predictor
counters remain insufficient promotion criteria for this design. ROB grouping
and BPU feedback timing form a sensitive closed loop; even 18 rare
group-boundary events can add 119 complete-run mispredictions. The successful
candidate passed the complete bzip2 gate, two independent smoke workloads, and
a repeated 50K run before promotion.

The result logs are under:

- `results/rob-compress-tage-mispredict-readfree/`
- `results/rob-compress-tage-train-skid/`
- `results/rob-compress-break-after-mul/`
- `results/rob-compress-break-after-mul-tail3/`
- `results/rob-compress-sc-lower-threshold/`
- `results/rob-compress-sc-no-bias/`
- `results/rob-compress-group-policy/abtb-wb8/`
- `results/rob-compress-group-policy/tage4way-352/`

Input SHA-256 for bzip2-derived:
`e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`.
