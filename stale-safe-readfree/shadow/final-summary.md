# SC read-free retained-shadow result

## Outcome

- Design: retained write-buffer shadow repair with a version coverage check
- SC SRAM physical port: unchanged single read/write port (`RW0`)
- OFF comparison: both generated SC SRAM macro files are byte-identical
- bzip2-derived IPC: **2.176869**
- Previous SC IPC target: **2.155968**
- Improvement over previous SC: **0.9695%**
- SC commit stale drop: **0**

## Validation

| Workload | Instructions | Cycles | IPC | Shadow repairs | SC stale drops | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| heat stress | 126151 | 26098 | 4.833742 | 69 | 0 | NEMU difftest, GOOD TRAP |
| heat native | 62526 | 15724 | 3.976469 | 99 | 0 | NEMU difftest, GOOD TRAP |
| bzip2-derived | 526258 | 241750 | 2.176869 | 3930 | 0 | NEMU difftest, GOOD TRAP |

All three emulator processes exited with status 0. No runtime assertion failure was found in their logs.

## SRAM-port evidence

The OFF and retained-shadow RTL contain the same two generated SC SRAM macro definitions:

- `sram_array_1p128x48m6s1h0l1b_bpu_sc`
- `sram_array_1p128x192m6s1h0l1b_bpu_sc`

Their corresponding SHA-256 hashes match between `build-off-readfree-shadow/rtl` and
`build-on-tage-sc-shadow2/rtl`. Each macro exposes one combined `RW0` port; no extra SRAM port was added.

The second shadow lookup is a combinational lookup over the existing register-based write buffer. A
`baseVersion` stored with each retained entry proves that the entry contains every row mutation needed to
advance the prediction snapshot to the current logical version. In-flight t1/t2 forwarding then preserves
the remaining update order before commit.

## Artifacts

- `heat-stress.log`, `heat-stress.time`
- `heat-native.log`, `heat-native.time`
- `bzip2-derived.log`, `bzip2-derived.time`
- Emulator: `rob-compress-readfree-training/build-on-tage-sc-shadow2/verilator-compile/emu`
