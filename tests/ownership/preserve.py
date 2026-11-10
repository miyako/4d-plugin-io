#!/usr/bin/env python3
"""Verify every baseline tracked file outside the intentional edit allowlist."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
baseline = "edb67437c40c510450893291dbcf606846c091f1"
allowed = {"4D Plugin API/Classes/C_PICTURE.cpp", "README.md"}
paths = subprocess.check_output(
    ["git", "-C", str(root), "ls-tree", "-r", "--name-only", baseline], text=True).splitlines()
for path in paths:
    if path not in allowed:
        original = subprocess.check_output(["git", "-C", str(root), "show", f"{baseline}:{path}"])
        assert (root / path).read_bytes() == original, f"Unexpected baseline edit: {path}"
print(f"Preserved all {len(paths) - len(allowed)} non-allowlisted baseline files")
