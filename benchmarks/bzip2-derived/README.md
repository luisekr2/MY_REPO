# bzip2-derived XiangShan benchmark

This is a bare-metal, in-memory workload derived from the redistributable
bzip2 1.0.3 source included with SPEC CPU2006's `redistributable_sources`.
It is not a SPEC CPU2006 benchmark and must not be reported as a SPEC score.

The default smoke harness generates 256 bytes of mixed compressible data, compresses it with
libbzip2, decompresses it, and verifies both a checksum and every output byte.
It avoids libc, files, and proprietary SPEC input data.

For practical RTL turnaround, the vendored source changes the internal block
unit from 100000 to 1024 bytes. The transform and entropy-coding paths remain,
but generated streams are intentionally not compatible with standard bzip2.
The unmodified 100 KiB-block version and even its 1 KiB input run were stopped
after 10 and 20 minutes respectively; a 4 KiB-block derivative was also
stopped after 25 minutes. These sizes were unsuitable for rapid A/B iteration.

The vendored upstream source and local derivative remain under the original
license in `vendor/bzip2-1.0.3/LICENSE`.
