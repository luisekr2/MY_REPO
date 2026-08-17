#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/scripts/experiment.env"
"$root/scripts/check-experiment-diff.py"
diff_so="$root/workloads/bin/riscv64-nemu-interpreter-so"
[[ -e "$diff_so" ]] || { echo "missing NEMU reference; run prepare-public-workloads.sh" >&2; exit 1; }
mapfile -t workloads < <(printf '%s\n' "$@")
if ((${#workloads[@]} == 0)); then
  mapfile -t workloads < <(find "$root/workloads/bin" -maxdepth 1 -type l -name '*.bin' -printf '%f\n' | sort)
fi
for workload in "${workloads[@]}"; do
  binary="$root/workloads/bin/$workload"
  [[ -e "$binary" ]] || { echo "missing workload: $binary" >&2; exit 1; }
  for variant in on off; do
    emu="$root/rob-compress-$variant/build/emu"
    [[ -x "$emu" ]] || { echo "missing emulator: $emu" >&2; exit 1; }
    for rep in $(seq 1 "$REPETITIONS"); do
      out="$root/results/raw/$variant/${workload%.bin}"
      mkdir -p "$out"
      /usr/bin/time -v -o "$out/run-$rep.time" \
        "$emu" -b 0 -e 0 -i "$binary" --diff "$diff_so" \
        --seed "$SEED" >"$out/run-$rep.log" 2>&1
    done
  done
done
"$root/scripts/normalize-results.py"

