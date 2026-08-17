# SC shadow-only across SPECint-derived workloads

These are bounded, bare-metal characterization workloads derived from the
available CPU2006 sources.  They are not official SPEC runs or SPEC scores.
All promoted measurements used seed 1 and NEMU differential checking.

## Guest performance

| Workload | Configuration | Cycles | IPC | Speedup vs OFF | Mispredicts |
|---|---|---:|---:|---:|---:|
| bzip2-derived | OFF | 248,004 | 2.121966 | baseline | 6,837 |
| | branch-isolated | 238,145 | 2.209830 | +4.14% | 6,812 |
| | **SC shadow-only** | **232,470** | **2.263780** | **+6.68%** | **6,526** |
| libquantum-derived | OFF | 306,671 | 3.044905 | baseline | 5,685 |
| | compression ON | 296,498 | 3.149377 | +3.43% | 5,540 |
| | branch-isolated | 297,638 | 3.137308 | +3.03% | 5,586 |
| | **SC shadow-only** | **292,921** | **3.187822** | **+4.69%** | **5,527** |
| mcf-derived | OFF | 685,069 | 1.662466 | baseline | 20,924 |
| | compression ON | 682,429 | 1.668890 | +0.39% | 21,309 |
| | branch-isolated | 678,580 | 1.678362 | +0.96% | 21,168 |
| | **SC shadow-only** | **676,102** | **1.684511** | **+1.33%** | **20,997** |

SC shadow-only is the fastest tested configuration on all three workloads.  It
beats branch-isolated by 2.44% on bzip2, 1.61% on libquantum, and 0.37% on mcf.

## Predictor-pressure comparison

| Workload | Configuration | TAGE conflicts | Training stalls | ROB entries saved | SC stale drops |
|---|---|---:|---:|---:|---:|
| libquantum | OFF | 33,642 | 91,993 | 0 | n/a |
| | SC shadow-only | 24,032 | 27,755 | 347,769 | 0 |
| mcf | OFF | 161,169 | 267,384 | 0 | n/a |
| | branch-isolated | 160,041 | 265,802 | 600,033 | n/a |
| | SC shadow-only | 149,002 | 175,876 | 601,468 | 0 |
| bzip2 | SC shadow-only | 22,369 | 25,306 | 335,397 | 0 |

The mcf result is important because mispredictions are 73 higher than OFF, yet
cycles improve by 1.33%.  The gain comes from 12,167 fewer TAGE conflicts and
91,508 fewer training stalls, not merely from misprediction count.  This
supports the original goal: ROB compression exposes/amplifies predictor
training contention, and stale-safe SC shadow training removes a material part
of that bottleneck.

## Validation

- All twelve completed rows above reached `HIT GOOD TRAP`.
- mcf SC shadow-only: 1,138,901 instructions, 676,102 cycles, IPC 1.684511.
- mcf SC `read_free_commit_stale_drop = 0`.
- mcf used 512 MiB RAM, matching its existing reference runs.
- Raw mcf SC output: `mcf-connected-16-r2/sc-shadow-only/run.log`.

## sjeng status

The first sjeng port was not admitted to the table.  Its linker reserved a
128 MiB heap inside `.bss`, causing hours of irrelevant zero initialization;
that was reduced to 1 MiB.  Even after that correction, the current engine
search exceeded the 90-minute comparison budget when three configurations ran
in parallel.  Those runs were intentionally aborted and are not performance
results.  A deterministic internal node budget is required before sjeng can be
used without changing the workload differently across configurations.
