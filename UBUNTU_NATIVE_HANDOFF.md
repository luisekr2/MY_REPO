# Ubuntu native migration handoff

This repository preserves the XiangShan ROB-compression and branch-predictor experiments that were developed under WSL. The large Verilator build trees, emulator binaries, toolchain packages, and repetitive raw logs are intentionally excluded; they must be rebuilt on the native Ubuntu host.

## Recommended starting point

Use `archive/tage-sc-shadow-final` for the latest integrated TAGE + SC shadow implementation on top of branch-isolated ROB compression. Use `archive/rob-compress-branch-isolated` when a predictor-unmodified baseline is required.

```bash
git clone --branch archive/tage-sc-shadow-final \
  https://github.com/luisekr2/MY_REPO.git xiangshan-rob-compress
cd xiangshan-rob-compress
git submodule update --init --recursive
```

The archived XiangShan branches share the upstream XiangShan history. They are independent experiment snapshots and should not be merged into `main` merely to inspect them.

## Preserved branches

- `archive/rob-compress-off-final`: instrumented compression-OFF reference.
- `archive/rob-compress-on-final`: instrumented compression-ON reference.
- `archive/rob-compress-branch-isolated`: branches and jumps excluded from ROB compression.
- `archive/tage-readfree-training`: TAGE read-free training candidate.
- `archive/tage-sc-shadow-final`: integrated TAGE work plus SC shadow-only/freshness candidates; recommended implementation source.
- `archive/ordered-wb-forward`: ordered write-buffer forwarding candidate.
- `archive/tage-operation-queue`: bank-local ordered TAGE operation-queue candidate.
- `archive/sc-targeted-candidates`: SC/JAL/AUIPC/way-freshness candidate history.
- `archive/specint-derived-workloads`: bare-metal mcf, libquantum, and sjeng sources, inputs, small ELF/BIN images, and curated SPECINT-derived reports.
- `archive/experiment-reports`: curated ROB-compression/TAGE/SC reports, including the OFF CoreMark HTML analysis.

## Retrieve reports and workloads

```bash
git clone --branch archive/experiment-reports \
  https://github.com/luisekr2/MY_REPO.git rob-experiment-reports

git clone --branch archive/specint-derived-workloads \
  https://github.com/luisekr2/MY_REPO.git specint-derived-workloads
```

## Build notes

- Previous builds used `NUM_CORES=1` for simulator execution configuration; this is not the host compile-job count.
- On the expanded-memory native host, begin compilation with six host jobs where supported, while monitoring peak memory.
- Rebuild Verilator emulators locally. The old `build-*` trees and the roughly 167 MB emulator artifact were not pushed.
- The mcf bare-metal image may require a 512 MiB simulated memory configuration.
- Consult `archive/experiment-reports:sc-shadow-only/OFF_TO_SC_SHADOW_ONLY_HARDWARE_CHANGES.md` and `archive/experiment-reports:sc-shadow-only/RESULTS.md` before modifying the recommended implementation.

## Integrity check after cloning

```bash
git ls-remote --heads https://github.com/luisekr2/MY_REPO.git 'refs/heads/archive/*'
git status --short --branch
```
