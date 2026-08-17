# libquantum RV64GC RTL result

## Outcome

The bounded 462.libquantum-derived workload passed differential execution on
the SC shadow-only XiangShan emulator.  Wall-clock time was **49:01.55**, close
to the requested one-hour class, with exit status 0 and `HIT GOOD TRAP`.

This is a characterization workload derived from the available CPU2006 source
tree; it is not an official SPEC run or an official SPEC score.

## Workload selection

Running the complete original Shor path with the minimum useful composite
input (`15 2`) exceeded 90 minutes in RTL before completion.  Simply reducing
that input further would remove the factoring behavior.  The bounded adapter
therefore retains the actual libquantum data structures and routines but gives
the slow RTL simulation an explicit, reproducible work bound.

Final parameters are `LQ_WIDTH=6` and `LQ_ROUNDS=4`.  Each run uses:

- the original sparse `quantum_reg` representation and hash lookup;
- complex-amplitude traversal and state expansion;
- the original Walsh transform, CNOT and conditional-phase gates;
- the original QFT and inverse QFT implementations;
- paired sigma-X gates, preserving the state while exercising updates;
- probability normalization, allocation-failure, and state-signature checks.

The one-round, six-qubit calibration took 13:11.50.  Four rounds yielded the
more useful 49-minute final workload without changing the quantum-state width.

## Final result

| Metric | Value |
|---|---:|
| Wall time | 49:01.55 |
| Maximum host RSS | 195,696 KiB |
| Guest instructions | 933,780 |
| Guest cycles | 292,921 |
| IPC | 3.187822 |
| Branch mispredictions | 5,527 |
| TAGE read conflicts | 24,032 |
| SC conflict candidates | 50,776 |
| SC snapshot trains | 6,611 |
| SC shadow repairs | 4,252 |
| SC identity fallbacks | 8,120 |
| SC stale fallbacks | 3,011 |
| SC pending fallbacks | 8,199 |
| SC pending forwards | 1,772 |
| SC commit stale drops | **0** |
| BPU training stalls | 27,755 |
| ROB entries saved | 347,769 |
| Decode-full stall cycles | 24,734 |
| I-buffer-full stall cycles | 42,603 |
| Control-redirect stall cycles | 138,301 |

The zero SC commit-stale-drop count is an important integrity result for this
read-free/shadow configuration.  The benchmark also generated substantial
TAGE/SC conflict and fallback activity, so the pass is not a trivial predictor
path test.

## Artifact and reproduction

Binary:
`/home/joe/work/specint2006-investigation/libquantum-baremetal/libquantum-rtl-1h.bin`

SHA-256:
`6673de0003f2a5d0a69ec6b917c442e213f304d0d49d169e5c675711229156b8`

Build:

```sh
cd /home/joe/work/specint2006-investigation/libquantum-baremetal
make -j1 clean
make -j1 LQ_WIDTH=6 LQ_ROUNDS=4
```

Run:

```sh
/usr/bin/time -v \
  /home/joe/work/01_GEN2_COMP/rob-compress-readfree-training/build-on-sc-shadow-only/verilator-compile/emu \
  -b 0 -e 0 \
  -i /home/joe/work/specint2006-investigation/libquantum-baremetal/libquantum-rtl-1h.bin \
  --diff /home/joe/work/01_GEN2_COMP/workloads/bin/riscv64-nemu-interpreter-so \
  --ram-size=64MB --seed 1 --force-dump-result
```

Raw outputs are in `sc-shadow-only/run.log` and `sc-shadow-only/run.time` next
to this report.

## Cross-configuration comparison

The same binary, seed 1, 64 MiB RAM, and NEMU differential checker were used
for all four configurations.  OFF, ON, and branch-isolated were run
simultaneously; therefore host wall time is not used to rank performance.
Guest cycles and IPC are the relevant deterministic comparison.

| Configuration | Cycles | IPC | Speedup vs OFF | Mispredicts | TAGE conflicts | Train stalls | ROB entries saved |
|---|---:|---:|---:|---:|---:|---:|---:|
| OFF | 306,671 | 3.044905 | baseline | 5,685 | 33,642 | 91,993 | 0 |
| Compression ON | 296,498 | 3.149377 | +3.43% | 5,540 | 32,686 | 91,795 | 546,293 |
| Branch-isolated | 297,638 | 3.137308 | +3.03% | 5,586 | 33,690 | 95,914 | 347,173 |
| SC shadow-only | **292,921** | **3.187822** | **+4.69%** | **5,527** | **24,032** | **27,755** | 347,769 |

SC shadow-only is 1.22% faster than ordinary compression ON and 1.61% faster
than branch-isolated.  Its advantage is not primarily greater ROB compression:
it saves far fewer ROB entries than ordinary ON.  Instead, it reduces TAGE
read conflicts by 28.6% and BPU training stalls by 69.8% versus OFF, while also
reducing mispredictions by 2.8%.  This is consistent with removing the
predictor training bottleneck caused or exposed by compression.

Ordinary ON obtains most of its 3.43% speedup from backend/ROB pressure relief,
despite barely changing training stalls.  Branch isolation is slightly slower
than ordinary ON here: it gives up compression opportunities and raises both
TAGE conflicts and training stalls.  This libquantum kernel therefore does not
support branch-isolated as the best universal policy; the shadow/read-free
predictor treatment recovers frontend performance without requiring the
largest ROB-entry reduction.

All four runs produced one `HIT GOOD TRAP`, exit status 0, and no bad/critical
trap.  The final configuration also recorded zero SC commit stale drops.
