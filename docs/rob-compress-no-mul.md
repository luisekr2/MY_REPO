# ROB compression: MUL isolation experiment

## Hypothesis

The full `RobCompressOnScShadowOnlyConfig` bzip2 run reported 831 compressed
groups containing a multiply and 445 `waitMulCycle` events.  Keeping multiply
instructions out of compressed groups might remove head-of-line completion
blocking at the cost of consuming more ROB entries.

The experiment added an opt-in `RobCompressMul` parameter and a
`RobCompressOnScShadowNoMulConfig`.  Defaults remained unchanged.

## Results

All runs used seed 1 and the same bzip2-derived image.

| run | baseline instructions | no-MUL instructions | change | resolve mispredicts |
| --- | ---: | ---: | ---: | ---: |
| 10K cycles | 10,313 | 10,333 | +0.194% | 97 -> 95 |
| 50K cycles | 110,652 | 110,375 | -0.250% | 1,248 -> 1,264 |

At 50K cycles, ROB entries saved fell from 77,695 to 77,243 and TAGE read
conflicts fell from 3,427 to 3,327, but the branch trajectory worsened.  The
candidate correctly produced zero `rob_compressed_groups_with_mul`; however,
`waitMulCycle` rose from 393 to 641 because standalone multiply entries were
now directly visible at the ROB head.  Isolation therefore did not remove the
underlying wait and sacrificed compression capacity.

## Decision

Rejected at the 50K gate.  The parameter, config, and datapath change were
removed.  Logs are retained in `results/rob-compress-no-mul/`.
