# SPECint2006-derived XiangShan RTL result

## Superseded result — do not use

Post-run audit found that these MCF runs reached an FP-disabled exception in
`flow_cost()` and then a double-trap that the emulator printed as GOOD TRAP.
Every log contains `HIT CRITICAL ERROR`. The table below is retained only as an
audit trail and must not be used as performance evidence. Corrected connected
MCF runs enable `mstatus.FS` and require zero critical-error messages.

## Original outcome (invalidated)

The retained-shadow TAGE+SC read-free design is the fastest tested design on
the adapted `429.mcf` workload. It improves IPC by **2.5398%** over OFF and by
**0.9219%** over branch-isolated ROB compression. All configurations executed
the same 88,867 committed instructions, passed NEMU difftest, reached GOOD
TRAP, and exited with status zero.

| Configuration | Instructions | Cycles | IPC | IPC vs OFF | Commit branch mispredicts | TAGE read conflicts | ROB entries saved |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| OFF | 88,867 | 51,274 | 1.733179 | baseline | 1,173 | 8,475 | 0 |
| ROB compression ON | 88,867 | 50,945 | 1.744371 | +0.6457% | 1,187 | 8,607 | 57,646 |
| Branch-isolated compression | 88,867 | 50,465 | 1.760963 | +1.6031% | 1,162 | 8,514 | 34,634 |
| TAGE read-free only | 88,867 | 51,854 | 1.713793 | -1.1185% | 1,226 | 2,244 | 35,485 |
| **TAGE+SC retained shadow** | **88,867** | **50,004** | **1.777198** | **+2.5398%** | **1,169** | **1,617** | **34,687** |

The final design is 3.6997% faster than the TAGE-only result. This matters
because TAGE-only substantially reduces physical read conflicts but performs
worse due to a rise in committed mispredictions. Adding stale-safe SC retained
shadow both eliminates that regression and exceeds the branch-isolated result.

## Final-design activity

| Counter | Value |
| --- | ---: |
| SC conflict candidates | 11,119 |
| SC snapshot trains | 1,891 |
| SC retained-shadow repairs | 956 |
| SC pending forwards | 317 |
| SC identity fallbacks | 3,143 |
| SC stale fallbacks | 918 |
| SC pending fallbacks | 1,860 |
| SC commit stale drops | **0** |

The zero commit-stage stale-drop count is the key correctness signal for the
retained-shadow policy in this run.

## Workload construction and validation

- Source: unmodified algorithm sources from `429.mcf` in the cloned CPU2006 tree.
- Input origin: the official `429.mcf` test input.
- RTL-sized input: the first eight trip records, preserving the input format and
  original trip values. The resulting MCF execution performs 24 network-simplex
  iterations.
- Native expected checksum: `14`.
- Bare-metal adaptation: input is embedded in `.rodata`; a bounded allocator and
  in-memory stdio shim replace the absent filesystem; GOOD TRAP is reachable only
  when the internal MCF checksum matches.
- Seed: 1; RAM: 64 MiB; NEMU difftest enabled; performance dump enabled.

The full official `429.mcf test` input was also compiled into a separate binary.
Its native reference run produced internal checksum `2,997,477`, stdout MD5
`2dc8bcf4a5e04423c69b17f7945e7411`, and `mcf.out` MD5
`9326ec86a8fa71f1b507839a5d58a2ad`; both output hashes match the repository
MANIFEST. Full test is impractical on this Verilator build: the 8-trip run alone
takes roughly 5.5 minutes, while the full native workload has 118,645 simplex
iterations.

This is therefore a **SPECint2006-derived RTL comparison**, not an official
SPECint2006 score and not representative of the geometric mean of all twelve
CINT2006 benchmarks.

## Input recovery

`401.bzip2/data/all/input/input.combined` was restored from the included
`input.combined.bz2`. The restored file is 53,779,568 bytes and its MD5,
`055a388172c50871c37d77906721f1d0`, matches MANIFEST exactly.

The repository still lacks `456.hmmer/ref/input/swiss41` and
`483.xalancbmk/ref/input/t5.xml`; neither has a recovery archive in the clone.
Their test/train sets remain usable.

## Artifacts

- Bare-metal source and binaries: `mcf-baremetal/`
- Full native reference: `native-check/`
- Per-configuration logs and timing: `results/mcf-8/`
- Restored CPU2006 tree: `spec-int-2006/`
