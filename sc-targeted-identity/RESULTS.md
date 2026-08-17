# SC mutation-target identity experiment

## Verdict

Rejected.  Ignoring Global/BW index mismatches when `commonHR.valid` is false
reduced identity fallbacks, but changed the SC state trajectory and added 304
bzip2-derived branch mispredictions.

| Workload/config | Cycles | IPC | Mispredicts | Identity fallback | Stale drop |
|---|---:|---:|---:|---:|---:|
| heat-256 SC baseline | 167,329 | 5.965816 | 49 | 0 | 0 |
| heat-256 targeted | 167,329 | 5.965816 | 49 | 0 | 0 |
| bzip2 SC baseline | 232,470 | 2.263780 | 6,526 | 7,053 | 0 |
| bzip2 targeted | 237,972 | 2.211420 | 6,830 | 1,344 | 0 |

Although Global/BW writes are disabled for invalid common history, their old
counter values still participate in SC sum/update decisions.  They therefore
remain part of the training identity and cannot safely be ignored.  The
candidate is 2.31% slower than SC shadow-only on bzip2 and is not promoted to
libquantum.

