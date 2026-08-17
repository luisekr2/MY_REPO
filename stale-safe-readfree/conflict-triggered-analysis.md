# ROB compression + stale-safe read-free TAGE iteration report

## Goal and fixed comparison conditions

- Benchmark: `benchmarks/bzip2-derived/bzip2-derived.bin`
- Reference model: `workloads/bin/riscv64-nemu-interpreter-so`
- Simulator arguments: `-b 0 -e 0 --seed 1 --force-dump-result`
- OFF baseline: 526,256 instructions, 248,004 cycles, IPC 2.121966
- Pass criteria: GOOD TRAP, NEMU difftest success, and IPC greater than 2.121966

All bzip2 rows below used the same binary, reference model, and seed. Small
dynamic-instruction-count differences are caused by microarchitectural timing
of the benchmark/runtime path; the successful row differs from OFF by only
3 instructions (0.00057%).

## Design iterations

| Iteration | Policy | Instructions | Cycles | IPC | vs OFF IPC | Mispredicts | TAGE read conflicts | Result |
|---|---|---:|---:|---:|---:|---:|---:|---|
| OFF | No ROB compression/read-free | 526,256 | 248,004 | 2.121966 | reference | 6,837 | 22,099 | GOOD TRAP |
| ROB compression + always read-free stale-safe | Use carried snapshot; reject/repair stale state | 526,259 | 251,284 | 2.094280 | -1.305% | 7,571 | 0 | GOOD TRAP, IPC fail |
| ROB compression + stale fallback | Fresh snapshot read-free; stale snapshot performs SRAM read | 526,256 | 249,389 | 2.110181 | -0.555% | 7,485 | 5,055 | GOOD TRAP, IPC fail |
| **ROB compression + conflict-triggered fresh-only** | **Read-free only for an actual prediction/training bank collision and only when every snapshot version is fresh** | **526,259** | **245,738** | **2.141545** | **+0.923%** | **7,339** | **4,543** | **GOOD TRAP, PASS** |

The successful policy is deliberately narrow. A non-conflicting training
request uses the original SRAM-read path. An actual conflicting request uses
the metadata snapshot only when the mutation-version check proves it fresh;
otherwise it retains the normal conflict/backpressure behavior. This avoids
letting snapshot-derived provider/alternate/allocation decisions perturb every
training request.

## Successful-run activity

| Counter | bzip2-derived | heat-calc smoke |
|---|---:|---:|
| Read-free snapshot trains | 6,794 | 240 |
| Stale snapshot read fallbacks | 0 | 0 |
| TAGE read conflicts | 4,543 | 312 |
| Stale provider updates dropped | 2 | 0 |
| Stale repairs enqueued/applied/dropped | 0 / 0 / 0 | 0 / 0 / 0 |
| ROB compressed instructions | 528,971 | 32,109 |
| ROB entries saved | 342,431 | 20,985 |

The bzip2 run saved 342,431 ROB entries while improving IPC. Read-free was
still materially exercised (6,794 training operations), so this is not simply
a disabled-feature control. The remaining 4,543 conflicts correspond to
requests that were not eligible for the fresh-only bypass.

## Correctness and reproducibility evidence

- bzip2-derived: GOOD TRAP at `0x80000014`; 526,259 instructions; 245,738
  cycles; IPC 2.141545; process exit status 0.
- heat-calc native smoke: GOOD TRAP at `0x80000022`; 62,526 instructions;
  15,724 cycles; IPC 3.976469; process exit status 0.
- Both commands supplied the NEMU shared-object reference with `--diff`.
- Emulator build used Verilator output split 5000. Large generated C++ files
  were compiled at `-j1`; remaining files at `-j3`. Verilator generation and
  large-file compile both completed with swap usage 0.

Raw evidence:

- `results/stale-safe-readfree/conflict-triggered/bzip2.log`
- `results/stale-safe-readfree/conflict-triggered/bzip2.time`
- `results/stale-safe-readfree/conflict-triggered/heat-native.log`
- `results/stale-safe-readfree/conflict-triggered/heat-native.time`

## Conclusion

The pass criterion is met: the conflict-triggered fresh-only read-free TAGE
design preserves ROB compression, passes GOOD TRAP/NEMU difftest on the fixed
bzip2-derived workload, and raises IPC from 2.121966 to 2.141545 (+0.923%).
