# ROB compression branch-isolated CoreMark smoke

- Repository: `/home/joe/work/01_GEN2_COMP/rob-compress-branch-isolated`
- Branch: `experiment/rob-compress-branch-isolated`
- Commit: `72a51c2261a25dd101f6b0bd783313c7edf55d50`
- Config: `RobCompressBranchIsolatedConfig`
- Workload: `coremark-2-iteration.bin`
- Seed: `1`
- Difftest: enabled
- Forced performance dump: enabled

## Validation

- Emulator build exit status: 0
- CoreMark exit status: 0
- GOOD TRAP: yes, PC `0x80001ca0`
- Final CRC: `0x72be`
- Branch/jump isolation assertion failure: none
- Difftest mismatch: none
- `rob_control_flow_shared_entry`: 0

## Results

| Metric | Branch isolated | Current ON | Delta |
|---|---:|---:|---:|
| Cycle count | 294,944 | 294,819 | +125 (+0.042%) |
| IPC | 2.250214 | 2.251185 | -0.000971 (-0.043%) |
| CoreMark iterations/sec | 879 | 878 | +1 |
| ROB input instructions | 795,892 | 797,034 | -1,142 |
| ROB allocated entries | 546,891 | 452,945 | +93,946 |
| ROB compressed instructions | 347,031 | 492,859 | -145,828 |
| ROB entries saved | 249,001 | 344,089 | -95,088 |

The current ON run is diagnostic context rather than a controlled statistical
comparison. Both runs use the same CoreMark image, seed, difftest reference,
and forced performance dump.

## Artifacts

- `coremark.log`: complete simulator and performance-counter output
- `coremark.time`: runtime resource usage and exit status
- `../../build/branch-isolated-emu.log`: elaboration and emulator build log
- `../../build/branch-isolated-emu.time`: Verilator model-generation resource usage
- `../../build/branch-isolated-emu-link.time`: final emulator compile/link resource usage
