# Branch-isolated ROB compression + SC retained-shadow read-free

## Verdict

This is the first tested configuration to exceed the branch-isolated baseline
on the fixed bzip2-derived workload while also reducing branch mispredictions.
It is the current performance leader for that workload.

The design deliberately disables TAGE read-free training. TAGE therefore keeps
the branch-isolated training policy, while only SC training may use the
stale-safe retained-shadow snapshot path. ROB compression and branch isolation
remain enabled.

## Fixed bzip2 comparison

| Configuration | Instructions | Cycles | IPC | Branch mispredicts | Result |
|---|---:|---:|---:|---:|---|
| OFF | 526,256 | 248,004 | 2.121966 | 6,837 | GOOD TRAP |
| branch-isolated | 526,260 | 238,145 | 2.209830 | 6,812 | GOOD TRAP |
| **SC shadow only** | **526,261** | **232,470** | **2.263780** | **6,526** | **GOOD TRAP** |

Relative to branch-isolated, SC shadow only improves IPC by **2.441%**, removes
5,675 cycles, and reduces branch mispredictions by 286. Relative to OFF, IPC is
6.683% higher.

The successful candidate reports:

- TAGE read conflicts: 22,369
- BPU training stalls: 25,306 (branch-isolated: 46,807)
- SC conflict candidates: 45,436
- SC snapshot trains: 6,029
- retained-shadow repairs: 3,278
- pending-update forwards: 1,597
- identity / stale / pending fallback: 7,053 / 3,384 / 7,495
- commit-stage stale drops: **0**
- ROB entries saved: 335,397

## Fast validation suite

| Workload | branch-isolated IPC | candidate IPC | IPC change | BI mispredicts | Candidate mispredicts | Result |
|---|---:|---:|---:|---:|---:|---|
| heat-native | 3.910808 | 3.966882 | +1.434% | 124 | 107 | GOOD TRAP |
| heat-stress | 4.865998 | 4.848232 | -0.365% | 89 | 94 | GOOD TRAP |
| bzip2-derived | 2.209830 | 2.263780 | +2.441% | 6,812 | 6,526 | GOOD TRAP |

All three runs used seed 1 and NEMU differential testing. No runtime assertion
failure or stale update drop occurred. The short TAGE-stress loop remains a
known weakness: SC state timing changes add five mispredictions and reduce IPC
by 0.365%. The candidate is therefore the bzip2 performance leader, not yet a
strict per-workload dominance result.

## SRAM-port and build validation

- Generated SC SRAM macros remain the same two single-RW-port macros:
  `sram_array_1p128x48m6s1h0l1b_bpu_sc` and
  `sram_array_1p128x192m6s1h0l1b_bpu_sc`.
- Both generated macro files are byte-identical to branch-isolated (matching
  SHA-256 hashes).
- The additional retained shadow is register/write-buffer lookup logic; no SC
  SRAM port is added.
- Configuration banner: `robCompressEn=true tageReadFree=false scReadFree=true`.
- Scala/RTL elaboration passed; maximum RSS 15.5 GiB, swap 0.
- Verilator emulator build passed at `-j1`; maximum RSS 34.1 GiB, swap 0.
- `git diff --check` passed.

## Artifacts

- Repository: `/home/joe/work/01_GEN2_COMP/rob-compress-readfree-training`
- Configuration: `RobCompressOnScShadowOnlyConfig`
- Emulator: `build-on-sc-shadow-only/verilator-compile/emu`
- Runtime logs: this directory
- Build logs: `results/build/sc-shadow-only-elab.log` and
  `results/build/sc-shadow-only-emu.log`

The MCF artifacts are stored outside `01_GEN2_COMP`, under
`/home/joe/work/specint2006-investigation`.  On the fixed
`mcf-baremetal/mcf-connected-16-r2.bin`, SC shadow-only completed 1,138,901
instructions in 676,102 cycles (IPC 1.684511), with 20,997 branch
mispredictions and zero stale drops.  It is 1.33% faster than OFF and 0.37%
faster than branch-isolated.
