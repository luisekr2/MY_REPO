# Read-free TAGE training versus ROB-compression OFF

All runs use the same benchmark binaries, seed 1, NEMU difftest, and forced
performance-counter dump. All four read-free runs terminate with exit status 0
and GOOD TRAP. Results are single-seed smoke measurements.

## End-to-end performance

| Benchmark | Metric | OFF | Branch isolated | Read-free | Read-free vs OFF |
|---|---|---:|---:|---:|---:|
| native | Instructions | 62,526 | 62,526 | 62,526 | identical |
| native | Cycles | 16,024 | 15,988 | 15,578 | -446 (-2.783%) |
| native | IPC | 3.902022 | 3.910808 | 4.013737 | +2.863% |
| TAGE-stress | Instructions | 126,151 | 126,151 | 126,151 | identical |
| TAGE-stress | Cycles | 25,999 | 25,925 | 25,936 | -63 (-0.242%) |
| TAGE-stress | IPC | 4.852148 | 4.865998 | 4.863934 | +0.243% |

## ROB behavior

| Benchmark | Configuration | ROB input | Allocated entries | Entries saved | Saved/input | Committed uops |
|---|---|---:|---:|---:|---:|---:|
| native | OFF | 66,463 | 66,463 | 0 | 0% | 62,527 |
| native | Branch isolated | 66,621 | 45,545 | 21,076 | 31.64% | 42,540 |
| native | Read-free | 65,968 | 45,031 | 20,937 | 31.74% | 42,550 |
| TAGE-stress | OFF | 130,635 | 130,635 | 0 | 0% | 126,152 |
| TAGE-stress | Branch isolated | 130,457 | 61,575 | 68,882 | 52.80% | 59,377 |
| TAGE-stress | Read-free | 130,595 | 61,646 | 68,949 | 52.80% | 59,379 |

Read-free retains the branch-isolated ROB compression rate. Control-flow
instructions sharing a compressed entry remain zero. ROB input counts include
wrong-path work and therefore vary when prediction behavior changes.

## TAGE and control flow

| Benchmark | Configuration | Completed trains | Read-free trains | Read-conflict cycles | Conditional TAGE mispredictions | Commit mispredicts | Allocations |
|---|---|---:|---:|---:|---:|---:|---:|
| native | OFF | 5,671 | n/a | 898 | 62 | 115 | 45 |
| native | Branch isolated | 4,999 | n/a | 1,351 | 67 | 126 | 41 |
| native | Read-free | 5,794 | 470 | 0 | 58 | 107 | 53 |
| TAGE-stress | OFF | 6,593 | n/a | 319 | 49 | 88 | 42 |
| TAGE-stress | Branch isolated | 6,597 | n/a | 306 | 49 | 86 | 42 |
| TAGE-stress | Read-free | 6,591 | 408 | 0 | 49 | 88 | 44 |

For native, eliminating the training-read conflict coincides with 795 more
completed trains than branch-isolated, 19 fewer commit mispredicts, and 410
fewer cycles. Against OFF it has 8 fewer commit mispredicts and 446 fewer
cycles. For TAGE-stress, conflict elimination does not improve prediction or
end-to-end performance; it is 11 cycles slower than branch-isolated and only 63
cycles faster than OFF.

## Supporting pipeline counters

| Benchmark | Configuration | Average ROB occupancy | ROB above half cycles | ROB walk cycles | Frontend flushes |
|---|---|---:|---:|---:|---:|
| native | OFF | 123.78 | 6,679 | 1,371 | 91 |
| native | Branch isolated | 70.99 | 19 | 1,135 | 98 |
| native | Read-free | 74.27 | 20 | 1,061 | 84 |
| TAGE-stress | OFF | 117.48 | 3,032 | 732 | 86 |
| TAGE-stress | Branch isolated | 56.44 | 22 | 665 | 82 |
| TAGE-stress | Read-free | 56.30 | 22 | 668 | 87 |

Average occupancy is the `utilization / clock_cycle` counter ratio. Physical
ROB occupancy falls under compression because several instructions share one
ROB entry; it is not evidence that fewer architectural instructions are in
flight.

## Interpretation and limitation

The native result supports the hypothesis that repeated TAGE training-read
conflicts can materially reduce performance in this short-loop layout. The
TAGE-stress result shows that removing the counter alone is not universally a
speedup: when the baseline has few conflicts and unchanged mispredictions, the
effect is effectively neutral.

The prototype stores 304 extra bits of TAGE table snapshot in BPU resolve
metadata and has no freshness/version validation. Consequently it changes
predictor training state relative to the reference implementation. GOOD TRAP
and difftest prove architectural correctness for these binaries, not predictor
semantic equivalence. Multi-seed and longer workloads, stale-snapshot
instrumentation, and a versioned fallback are required before treating the
native speedup as a safe design result.
