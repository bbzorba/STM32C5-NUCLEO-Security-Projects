#!/usr/bin/env python3
"""
tools/iar_build.py  —  IAR ARM command-line build helper
Compiles each C source file with iccarm, then links with ilinkarm,
and converts the .out to .hex and .bin with ielftool.

Usage (called from the Makefile iar-build target):
  python tools/iar_build.py \\
    --iccarm   <path/to/iccarm.exe>  \\
    --ilinkarm <path/to/ilinkarm.exe> \\
    --ielftool <path/to/ielftool.exe> \\
    --icf      <path/to/linker.icf>  \\
    --outdir   <output_directory>    \\
    --inc      <include_dir> [--inc ...] \\
    --define   <MACRO> [--define ...]  \\
    file1.c file2.c ...
"""

import argparse
import os
import subprocess
import sys

def run(cmd, label=""):
    print(f"  [{label}] " + " ".join(f'"{a}"' if " " in a else a for a in cmd))
    result = subprocess.run(cmd)
    if result.returncode not in (0, 1):   # IAR exits 1 on warnings-only, which is OK
        print(f"ERROR: {label} failed with exit code {result.returncode}", file=sys.stderr)
        sys.exit(result.returncode)
    # Treat exit code 1 as success (warnings only) — but check no error lines appear
    # (IAR compiler prints "Errors: none" on success)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--iccarm",   required=True)
    ap.add_argument("--ilinkarm", required=True)
    ap.add_argument("--ielftool", required=True)
    ap.add_argument("--icf",      required=True)
    ap.add_argument("--outdir",   required=True)
    ap.add_argument("--opt-level", choices=["size", "none"], default="size")
    ap.add_argument("--inc",      action="append", default=[])
    ap.add_argument("--define",   action="append", default=[])
    ap.add_argument("sources",    nargs="+")
    args = ap.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    # --- IAR C compiler flags ---
    opt_flag = "-On" if args.opt_level == "size" else "-Onone"

    icc_base = [
        "--cpu=Cortex-M33",
        "--fpu=none",
        "--endian=little",
        "-e",                   # IAR language extensions (needed for #pragma weak etc.)
        "--dlib_config", "normal",
        opt_flag,
        "--debug",
        "--diag_suppress=Pa082",  # suppress volatile ordering warning (harmless)
    ]
    for inc in args.inc:
        icc_base += [f"-I{inc}"]
    for d in args.define:
        # Accept either MACRO or DMACRO (make passes -DFOO style)
        macro = d.lstrip("D") if d.startswith("D") else d
        icc_base += [f"-D{macro}"]

    # --- Compile each source file ---
    obj_files = []
    for src in args.sources:
        obj_name = os.path.splitext(os.path.basename(src))[0] + ".o"
        obj_path = os.path.join(args.outdir, obj_name)
        cmd = [args.iccarm] + icc_base + [src, "-o", obj_path]
        run(cmd, label=f"CC {os.path.basename(src)}")
        obj_files.append(obj_path)

    # --- Link ---
    out_elf = os.path.join(args.outdir, "main.out")
    cmd = [
        args.ilinkarm,
        "--config", args.icf,
        "--entry", "Reset_Handler",
        "--map", os.path.join(args.outdir, "main.map"),
        "-o", out_elf,
    ] + obj_files
    run(cmd, label="LINK")

    # --- Convert to .hex ---
    out_hex = os.path.join(args.outdir, "main.hex")
    run([args.ielftool, "--ihex", out_elf, out_hex], label="HEX")

    # --- Convert to .bin ---
    out_bin = os.path.join(args.outdir, "main.bin")
    run([args.ielftool, "--bin",  out_elf, out_bin],  label="BIN")

    print(f"\nIAR build OK")
    print(f"  ELF: {out_elf}")
    print(f"  HEX: {out_hex}")
    print(f"  BIN: {out_bin}")

if __name__ == "__main__":
    main()
