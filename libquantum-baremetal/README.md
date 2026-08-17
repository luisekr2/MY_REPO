# Bounded bare-metal 462.libquantum workload

This port runs the original 462.libquantum implementation on the XiangShan
RV64GC bare-metal test environment.  It is intended for RTL experiments, not
for producing an official SPEC score.

The adapter deliberately exercises libquantum's characteristic sparse quantum
register and complex amplitudes through the original `quantum_walsh`, CNOT,
conditional-phase, QFT, inverse-QFT, and sigma-X routines.  `LQ_WIDTH` controls
the sparse state size and `LQ_ROUNDS` repeats the gate/QFT kernel.  A final
probability-normalization and state-signature check converts corruption or an
allocation failure into a bad trap.

The calibrated one-hour-class image uses six qubits and four rounds:

```sh
make -j1 clean
make -j1 LQ_WIDTH=6 LQ_ROUNDS=4
cp libquantum-test.bin libquantum-rtl-1h.bin
cp libquantum-test.elf libquantum-rtl-1h.elf
```

Use at least 64 MiB of emulated RAM.  The supplied runtime has a 32 MiB heap,
so the default smaller memory configuration is not valid for this image.

The preserved image `libquantum-rtl-1h.bin` has SHA-256:

```
6673de0003f2a5d0a69ec6b917c442e213f304d0d49d169e5c675711229156b8
```

