# ROB-compression ResolveQueue fast-drain experiment

## Objective

The SC-shadow ROB-compression leader can retire dense instruction groups and
produce bursty branch feedback. Its 16-entry `ResolveQueue` deliberately starts
rejecting new feedback at 12 occupied entries, while redirected entries already
known to be useless leave the sequential queue only one per cycle. The full
bzip2 baseline consequently spends 4,192 cycles at the threshold and drops
4,619 final resolves.

This experiment removes only a consecutive flushed prefix faster. It does not
increase queue capacity, admit past the existing threshold, reorder live
entries, or make a live BPU training request earlier.

## Implementation

The default-disabled `FastDrainFlushed` FTQ parameter enables an opt-in policy
in `RobCompressOnScShadowResolveFastDrainConfig`:

- preserve the normal head training handshake;
- after a successful head training request, also discard up to three following
  entries if every one is already marked flushed;
- otherwise discard up to four consecutive flushed entries from the head;
- stop at the first live, empty, or non-flushed entry;
- retain the original single-entry behavior in every existing configuration.

The four-entry width matches the queue's admission reserve. Two counters report
cycles that remove more than one entry and the number of entries removed beyond
the original one-per-cycle behavior:

- `resolveQueueFastDrainCycles`
- `resolveQueueFastDrainExtraEntries`

An initial implementation used ordinary Chisel `+` for
`1 + PopCount(three entries)`. A four-entry drain overflowed the two-bit result
to zero, cleared memory without advancing `deqPtr`, and stalled the queue after
the 10K gate. The final implementation uses carry-preserving `+&`. A dedicated
12K boundary regression then continued to train normally, retired 16,269
instructions, and reported 12 fast-drain cycles / 16 extra entries without an
assertion.

## Build validation

Scala formatting, repository-wide format checking, `xiangshan.compile`, test
RTL elaboration, and a fresh Verilator emulator build complete successfully.
Elaboration reports the same 137 pre-existing width warnings as the leader.

Across the 2,098 SimTop RTL files shared with the leader, generated
SystemVerilog grows from 279,136,256 to 279,171,317 bytes (+35,061, +0.0126%).
This excludes 33 stale FPGA-elaboration files in the reused build directory and
is a structural source-size proxy, not synthesized area, timing, or power. The
candidate adds no SRAM or queue entry; the growth is unrolled combinational
read/clear logic and performance instrumentation.

## bzip2 gates

The final 12K boundary test is functionally clean and proves the fixed policy is
active. At 50K, the candidate passes the promotion gate:

| Metric | SC-shadow leader | Fast-drain | Change |
|---|---:|---:|---:|
| Retired instructions | 110,652 | 111,429 | +777 (+0.702%) |
| IPC | 2.213040 | 2.228580 | +0.702% |
| Queue-full cycles | 1,004 | 983 | -21 (-2.09%) |
| Queue-full dropped resolves | 850 | 826 | -24 (-2.82%) |
| Fast-drain cycles / extra entries | 0 / 0 | 449 / 661 | +449 / +661 |
| TAGE read conflicts | 3,427 | 3,434 | +7 |
| BPU train stalls | 3,968 | 3,975 | +7 |
| Resolve / commit mispredicts | 1,248 / 1,113 | 1,250 / 1,109 | +2 / -4 |

## Complete smoke workloads

All workloads reach GOOD TRAP under NEMU difftest without assertion, abort, or
mismatch. Each final instruction and cycle count is identical to the leader:

| Workload | Leader cycles | Fast-drain cycles | Fast-drain events / extra entries | Result |
|---|---:|---:|---:|---|
| heat-native | 15,762 | 15,762 | 18 / 35 | GOOD TRAP |
| heat-stress-32 | 26,020 | 26,020 | 2 / 4 | GOOD TRAP |
| rob-capacity | 12,615 | 12,615 | 0 / 0 | GOOD TRAP |

## Full bzip2 result

The complete workload rejects the policy despite the positive 50K gate:

| Metric | SC-shadow leader | Fast-drain | Change |
|---|---:|---:|---:|
| Retired instructions | 526,261 | 526,261 | 0 |
| Cycles | 232,470 | 234,360 | +1,890 (+0.813%) |
| IPC | 2.263780 | 2.245524 | -0.806% |
| Queue-full cycles | 4,192 | 4,206 | +14 |
| Queue-full dropped resolves | 4,619 | 4,565 | -54 |
| Fast-drain cycles / extra entries | 0 / 0 | 3,784 / 5,660 | +3,784 / +5,660 |
| TAGE read conflicts | 22,369 | 22,406 | +37 |
| BPU train stalls | 25,306 | 25,707 | +401 |
| Resolve / commit mispredicts | 6,526 / 5,847 | 6,657 / 5,946 | +131 / +99 |

The candidate reaches GOOD TRAP under NEMU difftest without assertion or
critical error. It does discard 5,660 dead entries earlier and drops 54 fewer
resolves, but the changed feedback-admission timing adds 131 resolve-time
mispredictions and regresses cycles 0.813%.

## Decision

Do not promote fast-drain. The implementation and opt-in config are removed;
the counters remain supported by the log summary script for archived evidence.
Even reclaiming only already-flushed entries is not predictor-state neutral
once the freed queue positions admit feedback that the leader would drop.

## Rejected alternatives

- Eight-bank TAGE reduced full-run conflicts 20.4% but changed index mapping and
  regressed cycles 1.60%.
- Physically duplicated TAGE prediction/training SRAMs kept mapping fixed and
  removed all 10K read conflicts, but changed update timing and regressed the
  50K gate 8.27% (110,652 to 101,503 instructions).
- Mispredict-priority admission improved the 50K gate 0.92% and heat-native
  1.88%, but added 214 full-run resolve mispredictions and regressed cycles
  1.48%.

The fast-drain policy differs by reclaiming only feedback already proven dead,
rather than changing TAGE geometry, live training latency, queue capacity, or
feedback selection.

## Reproduction inputs

Input SHA-256 values:

- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- rob-capacity: `0761b6cd98b8aca153ef06123508d54b535e2ea7dcf091e04592ded48e66f0b9`
