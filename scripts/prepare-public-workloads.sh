#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
src="$root/rob-compress-on/ready-to-run"
dst="$root/workloads/bin"
mkdir -p "$dst"
find "$src" -maxdepth 1 -type f \( -name '*.bin' -o -name '*nemu*' \) -print0 |
while IFS= read -r -d '' file; do
  ln -sfn "$file" "$dst/$(basename "$file")"
done
(
  cd "$dst"
  find . -maxdepth 1 -type l -printf '%f\n' | sort |
  while IFS= read -r name; do sha256sum "$name"; done
) > "$root/workloads/SHA256SUMS"
echo "Prepared $(wc -l < "$root/workloads/SHA256SUMS") shared public files."
