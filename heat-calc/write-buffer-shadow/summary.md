# TAGE write-buffer shadow forwarding smoke result

Configuration:

- Source: `rob-compress-readfree-training`
- Config: `RobCompressOffReadFreeTrainingConfig`
- Seed: 1
- Read-free snapshot training: enabled
- TAGE entry WriteBuffer lookup/forwarding: enabled
- Verilator compile: single job; remaining generated C++ compiled with `OPT_FAST=-O0`

Both workloads passed NEMU difftest and reached `HIT GOOD TRAP`.

| Metric | Existing OFF + read-free | WriteBuffer shadow | Difference |
|---|---:|---:|---:|
| heat-calc native cycles | 15,812 | 15,790 | -22 (-0.14%) |
| heat-calc native IPC | 3.954338 | 3.959848 | +0.14% |
| heat-calc native branch mispredicts | 107 | 103 | -4 |
| heat-calc native read-free trains | 474 | 470 | -4 |
| heat-calc native TAGE read conflicts | 0 | 0 | 0 |
| heat-calc native forwarded table reads | n/a | 15,560 | n/a |
| TAGE-stress cycles | 25,951 | 25,951 | 0 |
| TAGE-stress IPC | 4.861123 | 4.861123 | 0 |
| TAGE-stress branch mispredicts | 86 | 86 | 0 |
| TAGE-stress read-free trains | 409 | 400 | -9 |
| TAGE-stress TAGE read conflicts | 0 | 0 | 0 |
| TAGE-stress forwarded table reads | n/a | 26,666 | n/a |

Interpretation:

- The forwarding path is active and both tests complete correctly.
- This smoke test does not establish a statistically meaningful performance
  improvement. TAGE-stress is cycle-identical to the existing OFF + read-free
  result; native improves by only 22 cycles.
- This revision forwards the retained TAGE entry (`valid`, `tag`, and taken
  counter). It does not yet make the separate useful-counter queue coherent,
  add mutation-age validation, or revalidate deferred allocations.
- Full stale-safe read-free training therefore remains incomplete.

Build-resource observation:

- Chisel/firtool generation peaked near 15 GiB RSS with a 16 GiB JVM limit.
- Verilator's default `--output-split 30000` produced individual C++ files that
  required about 30 GiB RSS even at `-O0`.
- Future rebuilds should reduce Verilator output splitting below 30,000 before
  raising compile parallelism.
