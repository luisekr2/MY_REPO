#!/usr/bin/env python3
"""Fail unless ON/OFF differ only in their named experiment config."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ON = ROOT / "rob-compress-on"
OFF = ROOT / "rob-compress-off"
ALLOWED = {
    "src/main/scala/top/Configs.scala",
    "src/main/scala/xiangshan/Parameters.scala",
    "src/main/scala/xiangshan/backend/BackendParams.scala",
    "src/main/scala/xiangshan/backend/CtrlBlock.scala",
    "src/main/scala/xiangshan/backend/datapath/RFReadArbiter.scala",
    "src/main/scala/xiangshan/backend/rename/CompressUnit.scala",
    "src/main/scala/xiangshan/backend/rename/Rename.scala",
}

def run(repo: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True)

def dirty(repo: Path) -> set[str]:
    return set(filter(None, run(repo, "diff", "--name-only", "HEAD").splitlines()))

def patch(repo: Path, path: str) -> str:
    text = run(repo, "diff", "--no-ext-diff", "HEAD", "--", path)
    text = "\n".join(line for line in text.splitlines() if not line.startswith("index ")) + "\n"
    text = text.replace("RobCompressOnConfig", "RobCompressConfig")
    text = text.replace("RobCompressOffConfig", "RobCompressConfig")
    if path.endswith("Configs.scala"):
        text = text.replace("RobCompressEn = true", "RobCompressEn = VALUE")
        text = text.replace("RobCompressEn = false", "RobCompressEn = VALUE")
    return text

errors = []
on_sha, off_sha = run(ON, "rev-parse", "HEAD").strip(), run(OFF, "rev-parse", "HEAD").strip()
if on_sha != off_sha:
    errors.append(f"base SHA differs: ON={on_sha}, OFF={off_sha}")
on_dirty, off_dirty = dirty(ON), dirty(OFF)
if on_dirty != off_dirty:
    errors.append(f"modified file sets differ: ON={sorted(on_dirty)}, OFF={sorted(off_dirty)}")
unexpected = (on_dirty | off_dirty) - ALLOWED
if unexpected:
    errors.append(f"unexpected modified files: {sorted(unexpected)}")
for path in sorted((on_dirty & off_dirty) & ALLOWED):
    if patch(ON, path) != patch(OFF, path):
        errors.append(f"non-experiment source difference in {path}")
on_sub = run(ON, "submodule", "status", "--recursive")
off_sub = run(OFF, "submodule", "status", "--recursive")
if on_sub != off_sub:
    errors.append("recursive submodule SHA/status differs")
if errors:
    print("EXPERIMENT DIFF CHECK: FAIL", file=sys.stderr)
    print("\n".join(f"- {e}" for e in errors), file=sys.stderr)
    sys.exit(1)
print(f"EXPERIMENT DIFF CHECK: PASS ({on_sha})")
