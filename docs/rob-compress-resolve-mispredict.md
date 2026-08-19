# ROB-compression mispredict-priority resolve training

## Objective

The SC-shadow ROB-compression leader drops branch-resolution feedback whenever
its 16-entry `ResolveQueue` reaches the conservative 12-entry burst threshold.
The full bzip2 baseline spends 4,192 cycles at that threshold and drops 4,619
resolves. Increasing the queue to 32 entries retained more updates but regressed
the 50,000-cycle gate by 0.58%, because it also lengthened the backlog of
low-value correct-prediction training.

The experimental `RobCompressOnScShadowResolveMispredictConfig` instead
preserved only a batch that contained a misprediction. The normal leader
schedule remained unchanged below the threshold and the configured queue
remained 16 entries. The implementation and config were removed after the full
run rejected the policy; this document preserves the measured evidence.

## Implementation and safety

At or above 12 occupied entries, the queue accepts an otherwise-dropped batch
only when:

- at least one final, non-filtered resolve is a misprediction;
- every new FTQ entry in the batch fits while leaving one physical entry free;
- the opt-in `PrioritizeMispredictWhenFull` parameter is enabled.

Admission is all-or-nothing for the cycle. This preserves the existing resolve
order, same-cycle FTQ merging, and enqueue-index calculation. Existing-entry
hits consume no additional slot. The 15/16 occupancy ceiling is required by
the queue's `deqPtr <= enqPtr` invariant.

An initial version allowed 16/16 occupancy. It passed the 10K and 50K gates,
but correctly failed the existing pointer-order assertion at cycle 72,082 of a
full bzip2 run. The final version changes the capacity test from `<= 16` to
`< 16`; all measurements below use the rebuilt safe version unless explicitly
identified as the rejected unsafe run.

Two counters make the policy observable:

- `resolveQueueFullMispredictBatchAccepted`
- `resolveQueueFullMispredictResolveAccepted`

## Build validation

Scala reformat, format checking, compilation, RTL elaboration, and a fresh
Verilator emulator build completed successfully. Elaboration reported the same
137 pre-existing width warnings as the leader and no new policy warning.

As a structural proxy, generated SystemVerilog grows from 279,136,256 to
279,142,015 bytes (+5,759, +0.0021%); the queue remains 16 entries. This is not
a synthesized area, timing, or power measurement.

## Short bzip2 gates

The 10K gate is performance-neutral and proves that the policy is active:

| Metric | SC-shadow leader | Final candidate | Change |
|---|---:|---:|---:|
| Retired instructions | 10,313 | 10,313 | 0 |
| IPC | 1.031300 | 1.031300 | 0 |
| Accepted mispredict batches / resolves | 0 / 0 | 1 / 1 | +1 / +1 |

At 50K the selected feedback changes later predictor behavior enough to improve
throughput, despite performing more TAGE training reads:

| Metric | SC-shadow leader | Final candidate | Change |
|---|---:|---:|---:|
| Retired instructions | 110,652 | 111,670 | +1,018 (+0.920%) |
| IPC | 2.213040 | 2.233400 | +0.920% |
| Accepted mispredict batches / resolves | 0 / 0 | 10 / 15 | +10 / +15 |
| TAGE read conflicts | 3,427 | 3,549 | +122 |
| BPU train stalls | 3,968 | 4,145 | +177 |
| Queue-full dropped resolves | 850 | 836 | -14 |
| Resolve / commit mispredicts | 1,248 / 1,113 | 1,261 / 1,110 | +13 / -3 |

Repeating the same gate with seed 2 reproduces every listed instruction,
counter, and IPC value exactly.

## Complete smoke workloads

All workloads reach GOOD TRAP under NEMU difftest without an assertion, abort,
or mismatch marker:

| Workload | Leader cycles | Candidate cycles | Change | Accepted batches / mispredicts | Result |
|---|---:|---:|---:|---:|---|
| heat-native | 15,762 | 15,465 | -297 (-1.884%) | 4 / 4 | GOOD TRAP |
| heat-stress-32 | 26,020 | 26,020 | 0 | 0 / 0 | GOOD TRAP |
| rob-capacity | 12,615 | 12,615 | 0 | 0 / 0 | GOOD TRAP |

On heat-native, resolve and commit mispredicts both fall by nine (107 to 98
and 109 to 100). Workloads that never reach the admission condition remain
exact cycle-for-cycle matches.

## Full bzip2 result

The complete run reverses both positive short gates and rejects the policy:

| Metric | SC-shadow leader | Safe candidate | Change |
|---|---:|---:|---:|
| Retired instructions | 526,261 | 526,259 | -2 |
| Cycles | 232,470 | 235,914 | +3,444 (+1.481%) |
| IPC | 2.263780 | 2.230724 | -1.460% |
| Resolve / commit mispredicts | 6,526 / 5,847 | 6,740 / 6,013 | +214 / +166 |
| TAGE read conflicts | 22,369 | 21,506 | -863 |
| BPU train stalls | 25,306 | 24,554 | -752 |
| Queue-full cycles / dropped resolves | 4,192 / 4,619 | 4,150 / 4,324 | -42 / -295 |
| Accepted batches / mispredict resolves | 0 / 0 | 66 / 72 | +66 / +72 |

The run reaches GOOD TRAP under NEMU difftest without assertion or critical
error, but the extra feedback changes the long predictor trajectory and adds
214 resolve-time mispredictions. Lower queue pressure and fewer training
conflicts therefore do not compensate for the 1.48% cycle regression.

## Decision

Do not promote mispredict-priority admission. Retain the original 16-entry,
12-entry-threshold admission semantics. The 50K and heat-native improvements
are useful counterexamples: predictor-policy changes require a complete
workload before promotion, even when queue counters improve and repeated short
runs are deterministic.

## Rejected TAGE banking alternative

Doubling TAGE banks from four to eight held table capacity and associativity
constant and reduced full-run read conflicts by 20.4% and training stalls by
18.1%. It nevertheless changed the index mapping, increased mispredicts from
6,526 to 6,718, and regressed bzip2 from 232,470 to 236,193 cycles (+1.60%).
The banking candidate is rejected; conflict reduction alone is not useful when
the predictor aliasing trajectory worsens.

## Reproduction inputs

Input SHA-256 values:

- bzip2-derived: `e81c573cece062d8f315e4145a7f2a06a10a21c63ac2a109119eea7ecedbac97`
- heat-native: `502129198ae59fe1b42f73637bfe0b419322454738fab5a0c55cde8acd0cbd05`
- heat-stress-32: `920ea7682b8837e135534fa6ceb190d0a272c96c143ae83a48f28a766ae2337a`
- rob-capacity: `0761b6cd98b8aca153ef06123508d54b535e2ea7dcf091e04592ded48e66f0b9`
