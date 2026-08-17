# SC shadow-only without the bias component

## Decision

Rejected at the heat-stress acceptance gate.  The source was restored to SC
shadow-only before any bzip2 promotion run.

The original bzip2 SC component counters suggested that bias was negatively
correlated in isolation (`correct=1,541`, `wrong=2,781`).  A configuration with
`BiasEnable=false`, TAGE read-free disabled, SC read-free enabled, branch
isolation, and ROB compression was therefore built.

- RTL elaboration: pass
- Verilator `-j6`: pass
- Configuration banner: pass (`scBias=false`, ROB size 352)
- Fixed heat-stress run: exceeded 33 minutes and was aborted
- SC shadow-only reference host time: 4:31.58

The isolated component sign counter is not a marginal-utility measure.  Bias
participates in the SC sum and adaptive threshold; removing it changes the
distribution and confidence calibration of every override.  The severe smoke
regression shows that component removal cannot be selected from sign accuracy
alone.

Artifacts:

- Candidate commit: `f63183a`
- Restoration commit: `ae1700a`
- Emulator: `rob-compress-sc-targeted/build-sc-nobias/verilator-compile/emu`
- Partial log: `results/sc-nobias/heat-stress/run.log`
- Resource timing: `results/sc-nobias/heat-stress/run.time`

The promoted source remains exact SC shadow-only.
