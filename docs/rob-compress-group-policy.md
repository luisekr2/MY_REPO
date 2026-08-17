# ROB compression group-policy experiment

## Objective

Improve the `RobCompressOnScShadowOnlyConfig` performance leader without
re-enabling branch, jump, or AUIPC sharing.  Previous experiments showed that
those instructions perturb predictor training timing while recovering little
useful ROB capacity.

This experiment changes one grouping property at a time:

1. cap a physical ROB entry at four architectural instructions;
2. keep DIV, REM, FDIV, and FSQRT in independent ROB entries;
3. stop compression at every FTQ boundary.

The original policy remains the default: group size eight, cross-FTQ grouping
enabled, and long-latency grouping enabled.

## Instrumentation

Rename now reports:

- compressed groups containing DIV/REM/FDIV/FSQRT;
- compressed groups containing MUL;
- compressed groups crossing an FTQ boundary;
- saved entries attributable to long-latency and cross-FTQ groups.

ROB now reports non-writebacked-head cycles split by compressed instruction
count.  `scripts/rob_compress_summary.py` extracts the common safety and
performance metrics from emulator logs.

## Acceptance gates

- NEMU difftest and GOOD TRAP with no critical-error marker on every workload;
- `rob_control_flow_shared_entry == 0`;
- `read_free_commit_stale_drop == 0`;
- no meaningful branch-misprediction increase;
- positive IPC geometric mean over SC shadow-only;
- no workload regression greater than 0.5% at final promotion.

## Results

The rebuilt Ubuntu baseline reproduced the archived leader exactly:

| Workload | Instructions | Cycles | IPC | Result |
|---|---:|---:|---:|---|
| heat-native | 62,526 | 15,762 | 3.966882 | GOOD TRAP |
| heat-stress-32 | 126,151 | 26,020 | 4.848232 | GOOD TRAP |
| bzip2-derived | 526,261 | 232,470 | 2.263780 | GOOD TRAP |

Adding the new counters did not change either heat workload by one guest
cycle.  The fast `maxGroupSize=4` comparison is:

| Workload | Baseline cycles | Group-4 cycles | IPC change | Mispredicts (base / g4) | Result |
|---|---:|---:|---:|---:|---|
| heat-native | 15,762 | 15,752 | +0.0635% | 107 / 107 | GOOD TRAP |
| heat-stress-32 | 26,020 | 26,134 | -0.4362% | 94 / 96 | GOOD TRAP |

The two-workload geometric mean is -0.1867%.  The stress regression remains
inside the 0.5% per-workload gate, so bzip2-derived was required for the final
three-workload decision.  Group-4 needed to improve bzip2 IPC by at least
0.3744% for that geometric mean to become positive.

Both heat workloads reported zero compressed groups containing DIV/REM or
FDIV/FSQRT.  The bzip2 ELF contains only one static `divw`, and its archived
baseline reports four divide-head wait cycles.  The no-long-latency candidate
is therefore inactive or bounded too tightly to justify a full emulator build.

Group-4 already splits many FTQ-spanning long groups.  On heat-stress it
reduced saved entries attributable to cross-FTQ groups from 17,716 to 5,420,
but also added two mispredictions and regressed IPC.  A stricter no-cross-FTQ
policy and the still more aggressive group-2 policy are held back unless the
bzip2 result supplies evidence that additional splitting is worthwhile.

### Fusion-boundary correction

The first group-4 bzip2 run exposed a correctness bug that the heat workloads
did not reach.  It stopped after 4,632 instructions and 6,785 cycles with
`robEntries 202 uopNum is overflow`.  A fixed lane-3 boundary could split a
macro-op fusion pair even though the cleared second instruction must share the
first instruction's ROB entry and writeback accounting.

The corrected policy replaces a nominal boundary through a fusion pair with
two boundaries, one immediately before and one immediately after the pair.
Rename also asserts that every normal fusion pair shares both a ROB entry and
a compression mask.  Generated RTL confirms that only the lane-3 fusion case
survives constant folding for group-4, and the SC SRAM hashes remain unchanged.
The corrected emulator reached the explicit 10,000-cycle bzip2 limit with
10,333 committed instructions, exit status zero, and no assertion, abort, or
critical-error marker.  This passes the targeted regression for the original
6,785-cycle failure.  Corrected heat-native and heat-stress runs reproduced
every guest metric from the initial group-4 runs.

The full corrected comparison is:

| Workload | Baseline cycles | Group-4 cycles | IPC change | Mispredicts (base / g4) | Result |
|---|---:|---:|---:|---:|---|
| heat-native | 15,762 | 15,752 | +0.0635% | 107 / 107 | GOOD TRAP |
| heat-stress-32 | 26,020 | 26,134 | -0.4362% | 94 / 96 | GOOD TRAP |
| bzip2-derived | 232,470 | 237,151 | -1.9746% | 6,526 / 6,757 | GOOD TRAP |

The three-workload IPC geometric mean is **-0.7862%**.  The bzip2 run retired
526,257 instructions versus 526,261 at baseline, while reaching the same GOOD
TRAP under NEMU difftest.  It reported no assertion, abort, critical error,
control-flow shared entry, or stale free-list commit drop.  The performance
result nevertheless fails both the 0.5% per-workload gate and the branch
misprediction gate: bzip2 added 4,681 cycles and 231 mispredictions.

## Decision

Do not promote `maxGroupSize=4`; retain the existing group-size-eight policy as
the performance leader.  Group-4 reduced ROB entries saved but perturbed the
branch-predictor timing enough to regress both stress and bzip2.  Group-2 and
no-cross-FTQ split groups still more aggressively, so the negative group-4
trajectory supplies no evidence to pay for their full builds.  The
no-long-latency candidate is also skipped because its measured opportunity is
negligible.  The configurable policies, diagnostics, and fusion-safe boundary
logic remain available for future experiments, with defaults preserving the
leader behavior.

Input SHA-256 values:

- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
