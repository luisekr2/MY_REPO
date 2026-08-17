# heat-calc ROB/TAGE benchmark

This is an RV64GC, libc-free adaptation of
`toddmaustin/mojo-v/bringup-bench/heat-calc`.

- `heat-calc-native.bin` preserves the FP64 stencil and loop structure.
- `heat-calc-tage-stress.bin` preserves the loop trip counts and adds
  independent ROB-compressible integer work to expose TAGE training/prediction
  bank conflicts.
- `rob-capacity.bin` puts 384 unrolled, destination-free integer operations
  behind each cold load.  This is just beyond the uncompressed ROB capacity:
  OFF must wait before issuing the next miss, while compression can expose
  multiple misses concurrently without physical-register or LSQ pressure.
- Smoke builds use `N=100` and `STEPS=32`.
- All binaries self-check and terminate through the NEMU trap instruction.

The Mojo-V encrypted `fp64e_t`, encrypted load/store instructions, key setup,
and disclosure code are intentionally excluded because the target XiangShan
configuration implements standard RV64GC rather than the Mojo-V ISA extension.
