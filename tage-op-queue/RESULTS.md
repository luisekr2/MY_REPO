# TAGE bank-local operation queue experiment

## Outcome

This prototype is **rejected**.  It compiles, elaborates, and passes NEMU
difftest on all three fast workloads, but it does not preserve branch-predictor
quality on the bzip2-derived workload.  The queue reduces SRAM read conflicts,
yet deferred training becomes visible too late and changes subsequent TAGE
predictions.

## Implemented configuration

- Source repository: `/home/joe/work/01_GEN2_COMP/rob-compress-tage-op-queue`
- Branch: `experiment/rob-compress-tage-op-queue`
- Modified source: `src/main/scala/xiangshan/frontend/bpu/tage/Tage.scala`
- Four bank-local payload queues, four entries per bank
- High-watermark backpressure at three entries (one physical slot reserved)
- Global bank-token FIFO to keep deferred operations in original order
- Only committed conditional-branch training can enter the queues
- Empty-queue bypass retains the original same-cycle training path when no
  read-bank conflict exists
- Prediction and redirect datapaths and the number of SRAM ports are unchanged
- Assertions check payload/token pairing, enqueue acceptance, and reserved-slot
  occupancy

## Build validation

- Scala compile: pass (`mill -i -j 1 -Djvm-xmx=24G`)
- RTL elaboration/firtool: pass, including combinational-loop checks
- Verilator emulator: pass, 912 objects, build parallelism 1
- Maximum RTL-generation RSS: approximately 29.5 GiB
- Build logs: `results/build/tage-op-queue-elab.log` and
  `results/build/tage-op-queue-emu.log`

## Fast workload results

All candidate runs used seed 1 and ended with `HIT GOOD TRAP` under NEMU
difftest.

| Workload | Configuration | IPC | Branch mispredicts | TAGE read conflict | BPU train stall |
|---|---:|---:|---:|---:|---:|
| heat-native | OFF | 3.902022 | - | - | - |
| heat-native | branch-isolated | 3.910808 | 124 | 1,351 | 4,031 |
| heat-native | op-queue | 3.942371 | 116 | 476 | 3,137 |
| heat-stress | OFF | 4.852148 | - | - | - |
| heat-stress | branch-isolated | 4.865998 | 89 | 306 | 685 |
| heat-stress | op-queue | 4.863184 | 89 | 224 | 568 |
| bzip2-derived | OFF | 2.121966 | 6,837 | 22,099 | - |
| bzip2-derived | branch-isolated | 2.209830 | 6,812 | 23,233 | 46,807 |
| bzip2-derived | op-queue | 2.148418 | 7,222 | 21,533 | 42,149 |

Relative to branch-isolated, the candidate changes IPC by +0.807% on
heat-native, -0.058% on heat-stress, and **-2.779%** on bzip2-derived.  On
bzip2-derived it reduces reported TAGE read conflicts by 1,700 and train-stall
cycles by 4,658, but adds 410 branch mispredictions.

The bzip2 candidate also reports:

- deferred training enqueued: 5,237
- queued-head conflict blocking: 19,244 cycles
- input backpressure: 8,885 cycles
- high-watermark occupancy: 9,214 cycles
- accumulated queue occupancy: 68,835 entry-cycles
- ROB entries saved: 341,329

## Diagnosis and decision

The experiment confirms that removing the SRAM arbitration stall alone is not
sufficient.  A committed training operation can wait behind prediction reads
and older queued operations.  During that interval, later predictions still
observe the pre-training TAGE state because the prediction path deliberately
has no pending-operation forwarding.  bzip2 exercises this window frequently:
the lower conflict/stall counts coexist with substantially more mispredictions.

The global token FIFO also introduces cross-bank head-of-line blocking.  A
different-bank drain scheduler could reduce queue occupancy, but it would
either reorder allocation/useful-reset effects or require sequence/forwarding
machinery.  More importantly, even an ideal scheduler cannot make a deferred
update visible at its original time without forwarding pending state into the
prediction lookup.  That contradicts the stated constraint that the prediction
path remain completely unchanged.

Therefore this operation-queue implementation should not replace the
branch-isolated baseline.  MCF was not run: the fast bzip2 acceptance criterion
(no misprediction increase and no IPC regression versus branch-isolated) had
already failed decisively.  The next viable design must either preserve
same-time training visibility with stale-safe pending-update forwarding, or
accept the original conflict/backpressure behavior; merely increasing queue
depth would hide backpressure while making the stale-training window longer.
