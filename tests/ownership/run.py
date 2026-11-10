#!/usr/bin/env python3
"""Compile actual production sources and the full unchanged SDK; never build a bundle."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[2])
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--expect-baseline-fail", action="store_true")
args = parser.parse_args()
source = args.source.resolve()
tests = Path(__file__).resolve().parent
args.output.mkdir(parents=True, exist_ok=True)
sdk_hash = hashlib.sha256((source / "4D Plugin API/4DPluginAPI.c").read_bytes()).hexdigest()
assert sdk_hash == "2721c3d4338cc2a350599ffcee2a8008abaa756acfd45dd710720c64ca8c8ce3"
flags = ["clang++", "-std=c++11", "-x", "objective-c++",
         "-include", "Carbon/Carbon.h", "-include", "Cocoa/Cocoa.h",
         "-I" + str(source / "4D Plugin API"),
         "-I" + str(source / "4D Plugin API/Classes"),
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g",
         "-Wno-deprecated-declarations", "-Wno-parentheses"]
sources = [source / "4D Plugin API/4DPluginAPI.c",
           *sorted((source / "4D Plugin API/Classes").glob("*.cpp")),
           source / "4DPlugin.cpp", tests / "engine.cpp"]
objects = []
for index, path in enumerate(sources):
    obj = args.output / f"{index}.o"
    extra = ["-include", str(tests / "io_stubs.h")] if path.name == "4DPlugin.cpp" else []
    subprocess.run(flags + extra + ["-c", str(path), "-o", str(obj)], check=True)
    objects.append(str(obj))
binary = args.output / "ownership"
subprocess.run(["clang++", "-fsanitize=address,undefined", *objects,
                "-framework", "Cocoa", "-framework", "Carbon", "-o", str(binary)], check=True)
failed = []
for case in ("path-alias-and-output", "setBytes-temporary", "copy-byte-units",
             "callback-empty-owners", "arrays-and-scalars", "dispatcher-no-io",
             "failure-and-handoff"):
    for empty in ("allocated-empty", "null-empty"):
        result = subprocess.run([str(binary), case, empty],
                                env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0",
                                     "UBSAN_OPTIONS": "halt_on_error=1"})
        if result.returncode:
            failed.append(f"{case}/{empty}")
if args.expect_baseline_fail:
    expected = [f"{case}/{empty}" for case in
                ("path-alias-and-output", "setBytes-temporary", "copy-byte-units",
                 "callback-empty-owners", "failure-and-handoff")
                for empty in ("allocated-empty", "null-empty")]
    print("Baseline failures: " + ", ".join(failed))
    sys.exit(failed != expected)
print("FAIL: " + ", ".join(failed) if failed else "All 14 ownership cases passed")
sys.exit(bool(failed))
