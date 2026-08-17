# Hazard-safe SC read-free training analysis

## Outcome

This iteration adds a two-stage in-flight row reservation check to SC read-free
training. A snapshot is used only when its identity and row versions match and
there is no older accepted mutation to any row consumed by that snapshot.
Otherwise training retries through the existing coherent SRAM/write-buffer read
path. No training queue, update coalescing, or stale-data replay is introduced.

The fallback counters were also moved off `t0_fire`, so stalled conflict cycles
are now observable. `read_free_pending_fallback` reports conflicts rejected by
the new in-flight reservation check.

## Safety results

All tests used Verilator assertions and NEMU differential testing with seed 1.

| Workload | Result | Instructions | Commit-stale drops | Pending hazards |
|---|---:|---:|---:|---:|
| heat-native | GOOD TRAP / NEMU pass | 62,526 | 0 | 350 |
| heat-tage-stress | GOOD TRAP / NEMU pass | 126,151 | 0 | 186 |
| bzip2-derived | GOOD TRAP / NEMU pass | 526,261 | 0 | 2,691 |

The previous SC prototype dropped 3 heat and 304 bzip updates at commit. The
new configuration observed zero drops in all three runs, including the directed
short-loop stress case.

## Performance

| Configuration | bzip cycles | bzip IPC | Relative to OFF |
|---|---:|---:|---:|
| ROB compression OFF | 248,004 | 2.121966 | baseline |
| TAGE read-free only | 245,738 | 2.141545 | +0.9227% |
| TAGE + SC, original late-drop prototype | 244,095 | 2.155968 | +1.6024% |
| TAGE + SC, hazard-safe | 245,559 | 2.143114 | +0.9966% |

Hazard-safe remains 0.0733% faster than TAGE-only and 0.9966% faster than OFF,
but is 0.5962% slower than the original SC prototype. This is the cost of
stalling/re-reading 2,691 in-flight hazards instead of using or dropping stale
snapshots.

For heat-native, hazard-safe improved from 15,397 cycles / 4.060921 IPC to
15,363 cycles / 4.069908 IPC while reducing commit-stale drops from 3 to 0.
Heat-tage-stress reached 26,134 cycles / 4.827083 IPC and saved 69,231 ROB
entries.

## Resource and implementation assessment

The safety addition stores two sets of SC bank/row identities and valid bits,
plus equality comparators. It does not duplicate the 626-bit snapshot, add SRAM
ports, add a training FIFO, or create update merge/flush semantics. This is a
smaller and easier-to-verify implementation than deferred full-request replay.

The long-run counters show that the remaining performance ceiling is dominated
by stale-version fallback (36,872 counted stall cycles) and identity fallback
(7,325), not the 2,691 new pending hazards. A future performance iteration
should use exact touched-counter forwarding or finer-grained epochs; removing
the reservation check would reintroduce silent training loss.

## Artifacts

- RTL/emulator: `rob-compress-readfree-training/build-on-tage-sc-hazardsafe/`
- Raw logs: `results/stale-safe-readfree/hazard-safe/`
- Modified safety logic: `src/main/scala/xiangshan/frontend/bpu/sc/Sc.scala`

Validation completed: Scala compilation/elaboration, Verilator build, project
format check, `git diff --check`, Verilator runtime assertions, three GOOD TRAP
runs, and NEMU differential testing.
