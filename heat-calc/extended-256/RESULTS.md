# Extended heat TAGE-stress (`STEPS=256`)

## Purpose and construction

The original stress workload used `STEPS=32` and retired only 126,151
instructions.  This run changes only the outer step count to 256, retaining the
same loop body, data size, branch cadence, and self-check.  The resulting
998,254-instruction workload is 7.91 times longer and is comparable in length
to the 934K-instruction libquantum test.

Binary SHA-256:
`645a7b570143afa345f6d394521bc8a59d2fd3c4f3ebc432032edde38a922b59`

## Results

Both configurations passed NEMU differential testing, produced `HIT GOOD
TRAP`, and exited with status 0.

| Metric | Branch-isolated | SC shadow-only | Change |
|---|---:|---:|---:|
| Instructions | 998,254 | 998,254 | 0 |
| Cycles | **167,225** | 167,329 | +104 |
| IPC | **5.969526** | 5.965816 | **-0.062%** |
| Branch mispredictions | **43** | 49 | +6 |
| TAGE read conflicts | 310 | **263** | -47 (-15.2%) |
| BPU training stalls | 687 | **281** | -406 (-59.1%) |
| Control-redirect stalls | **1,078** | 1,229 | +151 |
| ROB entries saved | 533,963 | 534,276 | +313 |
| SC commit stale drops | n/a | **0** | safe |

The original 32-step run showed a 95-cycle, 0.365% regression and five extra
mispredictions.  At 256 steps the absolute difference remains almost constant
at 104 cycles and six extra mispredictions, while the percentage regression
shrinks to 0.062%.  This shows that the short result was dominated by a small,
mostly fixed predictor warm-up/trajectory difference rather than a sustained
throughput loss.

SC shadow-only does remove substantial predictor training pressure in the
steady run, but this highly repetitive loop has little remaining training
pressure to monetize: only 687 branch-isolated train-stall cycles exist across
nearly one million instructions.  Six additional mispredictions add 151
control-redirect stall cycles and slightly outweigh the conflict savings.

## Reproduction

Build with one job:

```sh
cd /home/joe/work/01_GEN2_COMP/benchmarks/heat-calc-rob
make -j1 clean
make -j1 STEPS=256 heat-calc-tage-stress.bin
```

Raw logs are in the `branch-isolated` and `sc-shadow-only` subdirectories.

