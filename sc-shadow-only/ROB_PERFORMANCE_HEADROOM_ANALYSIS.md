# ROB performance headroom above SC shadow-only

## Goal

Find an optimization that raises performance above the fixed SC shadow-only
bzip2 result (232,470 cycles, IPC 2.263780) without adding SRAM ports or
degrading predictor correctness.

## Existing-counter evidence

The original SC shadow-only raw log already contains enough non-perturbing ROB
counters to decide whether another capacity/compression experiment is useful.

| Metric | Value | Fraction / interpretation |
|---|---:|---|
| ROB full cycles | 0 | Capacity never limits this run |
| Mean physical ROB occupancy | 49.93 / 352 | 14.18% |
| ROB empty cycles | 37,894 | 16.30% of execution |
| Load at non-writebacked head | 37,801 | 16.26% |
| Store at non-writebacked head | 12,437 | 5.35% |
| ALU at non-writebacked head | 15,538 | 6.68% |
| Branch at non-writebacked head | 2,993 | 1.29% |
| Jump at non-writebacked head | 3,085 | 1.33% |
| Physical ROB entries committed | 357,985 | Existing cross-entry commit works |
| Architectural instructions committed | 526,262 | Compression already expands at commit |
| ROB entries containing >1 instruction | 91,241 | Compression is active |
| Instructions represented by compressed entries | 259,518 | Substantial storage saving |

The commit distribution includes compressed sizes 1 through 8.  Therefore the
current implementation already commits across physical entries and accounts
for multiple architectural instructions per compressed entry.  A generic
"cross-entry commit packing" implementation would duplicate existing behavior.

## Rejected capacity-recovery experiments

| Candidate | Cycles | IPC | Mispredictions | Decision |
|---|---:|---:|---:|---|
| SC shadow-only | 232,470 | 2.263780 | 6,526 | Leader |
| Direct JAL compression | 234,078 | 2.248212 | higher trajectory cost | Reject |
| AUIPC-only compression | 236,164 | 2.228367 | 6,699 | Reject |

Even AUIPC-only recovered just 598 additional physical entries over the whole
run and shared only 117 control-flow-class entries, but changed rename/retire
timing enough to add 173 mispredictions.  Since the ROB never becomes full,
the recovered capacity cannot compensate for the trajectory perturbation.

## Bottleneck decision

Do not implement more aggressive ROB grouping for this acceptance workload.
It has essentially zero cycle-saving capacity headroom and a demonstrated
predictor-trajectory risk.

The next candidate above SC shadow-only should preserve its ROB grouping
byte-for-byte and target frontend supply:

1. Preserve TAGE/SC prediction, redirect, and committed-training identity.
2. Do not add an SRAM port.
3. Improve only a provably reconstructable SC fallback class; never consume a
   stale or identity-mismatched snapshot.
4. Require zero `read_free_commit_stale_drop` and no increase over 6,526 branch
   mispredictions before promotion.

The raw source is `results/sc-shadow-only/bzip2-derived.log`.  No new RTL was
needed for this decision, avoiding a redundant instrumentation build that
could itself perturb timing.
