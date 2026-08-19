# ROB compression RAB recovery-width experiment

## Objective

Reduce redirect recovery latency caused by ROB compression.  A physical ROB
entry can represent up to eight architectural instructions and therefore more
than eight rename destinations.  The leader walks eight physical ROB entries
and drains only eight RAB destinations per cycle, leaving a serialized RAB
tail after the ROB walk finishes.

The SC-shadow leader's full bzip2 run reports:

- `walkCycleTotal = 24,718`;
- `waitRabWalkEnd = 4,444`;
- `recovery_stall = 24,736`.

The experiment widens the RAB/RAT/free-list recovery datapath to 16 while
keeping normal ROB commit width at eight.  A second policy preserves eight-wide
ROB/special walking and enables the extra lanes only for the residual RAB tail.

## Width-parameterization fixes

The original implementation implicitly assumed
`RabCommitWidth == RenameWidth == RobCommitWidth`.  Independent recovery width
required these fixes:

- size RAB walk requests and free-list recovery ports by `RabCommitWidth`;
- keep speculative RAT writes sized by steady-state `RenameWidth`;
- keep difftest architectural commit ports sized by `CommitWidth`;
- let the free-list one-hot head advance by the larger of rename and RAB widths;
- bound vector-exception RAB/RAT patch loops by vector-map capacity;
- apply the tail-only width consistently to both RAB and VTypeBuffer recovery.

Before the free-list head fix, bzip2 asserted at guest cycle 4,483 with
`wrong one-hot reg`.  The rebuilt designs pass 10K and 50K NEMU difftest runs
without assertion, abort, or critical-error markers.

## Predictor-capacity candidates

Two attempts to spend an assumed smaller-ROB budget on TAGE were rejected:

| Candidate | bzip2 scope | Baseline IPC | Candidate IPC | Change | Mispredicts (base / cand.) |
|---|---:|---:|---:|---:|---:|
| ROB208 + all TAGE tables 2x sets | full | 2.263780 | 2.239505 | -1.0724% | 6,526 / 6,654 |
| ROB208 + all TAGE tables 2x ways | 50K cycles | 2.213040 | 2.194320 | -0.8459% | 1,248 / 1,251 |

The important diagnosis was not predictor capacity.  Every ROB208 50K
candidate reports `stall_cycle_rob` around 1,855--1,861 cycles, while the
352-entry leader reports zero.  Mean occupancy (49.93 entries) hid short
full-ROB bursts.  `disallow_enq_cycle` was also insufficient because it is a
RAB enqueue counter, not the ROB-full counter.  `stall_cycle_rob` is now a
mandatory capacity gate.

## RAB policy matrix

The fixed-cycle bzip2 comparison separates ROB capacity from recovery policy:

| ROB / recovery policy | Instructions at 50K | IPC change | Mispredicts | Recovery stall | RAB-tail wait | ROB-stall cycles |
|---|---:|---:|---:|---:|---:|---:|
| 352 / baseline 8-wide | 110,652 | -- | 1,248 | 5,462 | 1,095 | 0 |
| 208 / full RAB16 | 110,338 | -0.284% | 1,254 | 5,008 | 755 | 1,861 |
| 208 / tail-only RAB16 | 109,513 | -1.029% | 1,248 | 4,990 | 799 | 1,861 |
| 352 / full RAB16 | 109,398 | -1.133% | 1,256 | 5,081 | 762 | 0 |
| 352 / tail-only RAB16 | 110,104 | -0.495% | 1,250 | 5,131 | 802 | 0 |

Widening does remove the targeted recovery tail, but full-width walking changes
the long-run predictor/memory trajectory too aggressively.  Shrinking the ROB
adds a separate capacity loss.  The only combination inside the predeclared
0.5% per-workload gate is the 352-entry tail-only policy.

## Tail-only ROB352 complete-workload results

All runs use seed 1, the same NEMU reference, and GOOD TRAP acceptance.

| Workload | Baseline cycles | Candidate cycles | IPC change | RAB-tail wait (base / cand.) | Mispredicts (base / cand.) | Result |
|---|---:|---:|---:|---:|---:|---|
| heat-native | 15,762 | 15,656 | +0.6771% | 287 / 165 | 107 / 108 | GOOD TRAP |
| heat-stress-32 | 26,020 | 26,073 | -0.2033% | 280 / 158 | 94 / 102 | GOOD TRAP |
| bzip2-derived | 232,470 | 235,723 | -1.3806% | 4,444 / 3,744 | 6,526 / 6,754 | GOOD TRAP |

The three-workload IPC geometric mean is **-0.3058%**.  Full bzip2 removes 700
RAB-tail wait cycles but adds 228 branch mispredictions and 3,253 total cycles.
It fails both the 0.5% per-workload gate and the positive-geometric-mean gate.

## Decision

Do not promote any RAB16 policy.  Keep the existing 352-entry SC-shadow leader
and its eight-wide RAB recovery.  The experiment establishes three reusable
results:

1. ROB compression does create a real RAB recovery tail, and wider recovery
   consistently reduces it.
2. Direct recovery-timing changes perturb long-run predictor and memory
   trajectories enough to erase the local gain on bzip2.
3. ROB208 is not a safe area trade despite low mean occupancy; burst full
   cycles must be checked with `stall_cycle_rob` before resizing.

The opt-in configs and generalized width fixes remain for reproducibility and
future workloads.  Defaults are unchanged.

## RTL-size proxy

Retaining the 352-entry ROB means this candidate buys performance with wider
recovery logic rather than ROB-area savings.  Generated SystemVerilog grows
from 279,136,256 to 282,987,475 bytes (+1.38%).  Notable contributors are
`RenameBuffer`, `MEFreeList`, `Rename`, and the RAT ports.  These byte counts
are a structural proxy only, not synthesized area, timing, or power.

Input SHA-256 values:

- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
