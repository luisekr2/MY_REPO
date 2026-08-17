#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/scripts/experiment.env"
out="$root/results/environment-manifest.txt"

{
  echo "generated_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "expected_sha=$EXPERIMENT_SHA"
  echo "kernel=$(uname -srmo)"
  echo "cpu_count=$(nproc)"
  echo "memory_bytes=$(awk '/MemTotal/ {print $2 * 1024}' /proc/meminfo)"
  echo "swap_bytes=$(awk '/SwapTotal/ {print $2 * 1024}' /proc/meminfo)"
  for tool in git java mill verilator clang make cmake ninja python3; do
    path=$(command -v "$tool" 2>/dev/null || true)
    echo "tool.$tool=${path:-MISSING}"
  done
  for variant in on off; do
    repo="$root/rob-compress-$variant"
    echo "repo.$variant.sha=$(git -C "$repo" rev-parse HEAD)"
    git -C "$repo" submodule status --recursive |
      sed "s/^/repo.$variant.submodule=/"
  done
} > "$out"
echo "$out"

