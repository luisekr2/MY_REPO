#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/scripts/experiment.env"
export JAVA_HOME="$root/tools/jdk-21"
export CCACHE_DIR="$root/tools/ccache"
export PATH="$JAVA_HOME/bin:$root/tools/bin:$PATH"
mkdir -p "$CCACHE_DIR"
mode=${1:-verilog}
variant=${2:-both}
[[ "$mode" == verilog || "$mode" == emu ]] || { echo "usage: $0 {verilog|emu} [on|off|both]" >&2; exit 2; }
[[ "$variant" == on || "$variant" == off || "$variant" == both ]] || { echo "invalid variant" >&2; exit 2; }
"$root/scripts/check-experiment-diff.py"

build_one() {
  local name=$1 config=$2 repo="$root/rob-compress-$1"
  mkdir -p "$root/results/build"
  /usr/bin/time -v -o "$root/results/build/$name-$mode.time" \
    env XIANGSHAN_GIT_SHA="$EXPERIMENT_SHA" NOOP_HOME="$repo" \
    make -C "$repo" "$mode" CONFIG="$config" EMU_THREADS="$EMU_THREADS" \
      JVM_XMX=28G OBJCACHE=ccache -j"$BUILD_JOBS" 2>&1 |
    tee "$root/results/build/$name-$mode.log"
}
if [[ "$variant" == on || "$variant" == both ]]; then build_one on "$ON_CONFIG"; fi
if [[ "$variant" == off || "$variant" == both ]]; then build_one off "$OFF_CONFIG"; fi
