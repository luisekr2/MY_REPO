# ROB compression with four-way TAGE and a full-size ROB

## Objective

Improve the `RobCompressOnScShadowOnlyConfig` leader without changing ROB
group boundaries, predictor indexes, or SC training policy.  The earlier
resource-trade experiment doubled TAGE associativity while shrinking the ROB
from 352 to 208 entries.  Its 50K result was rejected at IPC 2.194320, but that
design also accumulated 1,861 ROB-full stall cycles.  The mixed experiment did
not identify the standalone TAGE effect.

`RobCompressOnScShadowTage4WayConfig` removes that confound:

- ROB size remains 352 and compression remains enabled;
- maximum group size remains eight, including cross-FTQ and long-latency
  grouping;
- conditional branches, jumps, and AUIPC keep the leader's isolation policy;
- TAGE read-free training stays disabled and SC shadow training stays enabled;
- all eight TAGE tables keep their 512 sets per bank and history lengths, while
  associativity increases from two to four ways.

Each table therefore changes from 4,096 to 8,192 entries without changing the
folded-history index width.  This preserves the set mapping that the rejected
two-times-set candidate changed.

## Validation

The Scala format check, `xiangshan.compile`, RTL elaboration, and Verilator
build completed successfully.  Elaboration reported the same 137 pre-existing
width warnings as the leader.  The config banner confirmed ROB size 352, TAGE
table sizes `[8192,8192,8192,8192,8192,8192,8192,8192]`, and four ways per
table.

All functional runs used NEMU difftest and seed 1.  Complete workloads reached
GOOD TRAP without assertion, abort, or critical-error markers.  Fixed-cycle
gates terminated at their requested limit as expected.

The 10K bzip2 gate exactly matched the leader at 10,313 retired instructions
and IPC 1.031300.  At 50K, the isolated candidate passed the promotion gate:

| Metric | Leader | ROB208 + TAGE 4-way | ROB352 + TAGE 4-way |
| --- | ---: | ---: | ---: |
| Instructions | 110,652 | 109,716 | 111,372 |
| IPC | 2.213040 | 2.194320 | 2.227440 |
| IPC vs. leader | -- | -0.846% | +0.651% |
| Resolve mispredicts | 1,248 | 1,251 | 1,250 |
| Commit mispredicts | 1,113 | 1,115 | 1,110 |
| ROB-full stall cycles | 0 | 1,861 | 0 |
| TAGE training stalls | 3,968 | 3,732 | 3,832 |
| TAGE read conflicts | 3,427 | 3,258 | 3,340 |

A second 50K run reproduced every metric extracted by
`scripts/rob_compress_summary.py`, including 111,372 instructions, IPC
2.227440, 1,250 resolve mispredicts, and 1,110 commit mispredicts.

## Complete workload results

| Workload | Leader cycles / IPC | Candidate cycles / IPC | IPC change | Resolve mispredicts | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| heat-native | 15,762 / 3.966882 | 15,762 / 3.966882 | 0.000% | 107 / 107 | GOOD TRAP |
| heat-stress-32 | 26,020 / 4.848232 | 26,020 / 4.848232 | 0.000% | 94 / 94 | GOOD TRAP |
| bzip2-derived | 232,470 / 2.263780 | 231,680 / 2.271474 | +0.340% | 6,526 / 6,516 | GOOD TRAP |

The three-workload IPC geometric mean is **+0.113%**, with no workload
regression.  Complete bzip2 retires 526,255 instructions versus 526,261 in the
leader while reaching the same GOOD TRAP.  It saves 790 cycles, reduces commit
mispredicts from 5,847 to 5,812, reduces queue-full cycles from 4,192 to 4,151,
and reduces `TAGEMissBubble` from 235,664 to 234,008.

## Hardware-cost proxy

This is a performance configuration, not an area-neutral replacement.  The
generated design doubles TAGE entry SRAM instances from 64 to 128 and useful
counter SRAM instances from 64 to 128.  The additional 32,768 entries require
622,592 state bits for valid, 13-bit tag, three-bit taken counter, and two-bit
useful counter state.  Two additional ways in the per-table mutation-version
tracking add another 12,288 bits, for about **634,880 added state bits**
(79,360 bytes) before peripheral mux, replacement, write-buffer, and metadata
logic.

As a structural proxy, the freshly generated `SimTop.sv` grows from 1,977,301
to 2,173,102 bytes (+195,801, +9.90%).  This is not a synthesis area or timing
estimate.  Four-way tag comparisons and selection can affect the TAGE timing
path, so synthesis timing/area and power analysis remain required before a
physical-design promotion.

## Decision

Keep `RobCompressOnScShadowTage4WayConfig` as the opt-in measured performance
leader.  Keep `RobCompressOnScShadowOnlyConfig` as the lower-cost reference.
The result demonstrates that the earlier TAGE associativity rejection was
caused by combining it with an undersized ROB, not by associativity alone.

Result logs and CSV summaries are under
`results/rob-compress-group-policy/tage4way-352/`.

Input SHA-256 values:

- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
