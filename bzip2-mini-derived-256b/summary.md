# bzip2-mini-derived 256 B result

The workload is derived from redistributable bzip2 1.0.3 source. It uses a
1 KiB internal block unit for practical RTL turnaround and is not a SPEC
CPU2006 benchmark or a standard-compatible bzip2 implementation.

All configurations used seed 1 and passed the benchmark's compress,
decompress, checksum, byte comparison, NEMU difftest, and GOOD TRAP checks.

| Metric | OFF | Branch-isolated | Branch-isolated + read-free TAGE |
|---|---:|---:|---:|
| Committed instructions | 526,256 | 526,260 | 526,255 |
| Cycles | 248,004 | 238,145 | 252,385 |
| IPC | 2.121966 | 2.209830 | 2.085128 |
| ROB input instructions | 1,101,335 | 1,082,128 | 1,137,795 |
| ROB allocated entries | 1,101,335 | 741,943 | 785,456 |
| ROB entries saved | 0 | 340,185 | 352,339 |
| ROB half-full cycles | 34,836 | 10,157 | 9,909 |
| ROB stall cycles | 121 | 121 | 123 |
| Branch mispredicts | 6,837 | 6,812 | 7,617 |
| TAGE read conflicts | 22,099 | 23,233 | 0 |
| Read-free snapshot trains | 0 | 0 | 21,357 |
| SC write/read bank conflicts | 56,277 | 57,241 | 59,629 |

The speedup does not come from eliminating ROB-full dispatch stalls; those
were already negligible. Compression lowers average ROB occupancy and allows
the core to retain more in-flight work across the branch-heavy compression
code. Changed speculative timing also changes TAGE/SC conflict counts, so
read-free predictor training remains an independent optimization target.

The current read-free TAGE implementation is not performance-safe on this
workload. Relative to branch-isolated compression, it removes all 23,233 TAGE
read conflicts but increases cycles by 5.98%, lowers IPC by 5.64%, and
increases committed branch mispredicts by 11.82%. Detailed reason counters
show TAGE-attributed mispredicts rising only 3,149 to 3,174, while
BTB-attributed mispredicts rise 989 to 1,682 and SC-attributed mispredicts rise
1,951 to 2,013. This is consistent with stale snapshots or stale allocation
state perturbing later predictor/BTB training and speculative timing. A
versioned or delta-merged update scheme is required before enabling read-free
training generally.
