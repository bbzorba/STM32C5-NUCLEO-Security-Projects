#!/usr/bin/env python3
"""
Check one module's build output against the STM32C562RE memory map.

  python3 tools/ci/verify_build.py <build dir> <module dir> <FLASH_ADDR> [--manifest FILE]

<build dir> holds main.elf + main.bin, <module dir> is e.g. Projects/App_Demo
(its linker.ld is read), FLASH_ADDR is what `make print-FLASH_ADDR` reports.

Checks:
  1. linker.ld FLASH ORIGIN == FLASH_ADDR    (make flash writes where it was linked)
  2. lowest flash LOAD segment == FLASH_ADDR (ELF matches the linker script)
  3. main.bin == flash image of the ELF       (objcopy output is complete)
  4. size <= linker FLASH LENGTH and <= the 512 KB chip
  5. vector table: initial MSP in SRAM, Thumb reset vector inside the image

Prints a report, appends a JSON line to --manifest and a row to
$GITHUB_STEP_SUMMARY when set. Exit code 1 on any failed check.
"""

from __future__ import annotations

import os
import re
import sys
import json
import zlib
import struct
import hashlib
import argparse
from pathlib import Path

CHIP_FLASH_START = 0x08000000
CHIP_FLASH_SIZE  = 512 * 1024          # STM32C562RE
SRAM_START       = 0x20000000
SRAM_END         = 0x20020000          # 128 KB
ACTIVE_SLOT_ADDR = 0x08010000          # Projects/Bootloader active slot
ACTIVE_SLOT_SIZE = 176 * 1024
PT_LOAD          = 1


def parse_size(text: str) -> int:
    m = re.fullmatch(r"\s*(0x[0-9a-fA-F]+|\d+)\s*([KkMm]?)\s*", text)
    if not m:
        raise ValueError(f"cannot parse size '{text}'")
    value = int(m.group(1), 0)
    return value * {"": 1, "k": 1024, "m": 1024 * 1024}[m.group(2).lower()]


def linker_flash_region(linker: Path) -> tuple[int, int]:
    """ORIGIN and LENGTH of the FLASH region in a GNU ld script."""
    text = re.sub(r"/\*.*?\*/", "", linker.read_text(), flags=re.S)
    m = re.search(r"^\s*FLASH\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,]+),\s*LENGTH\s*=\s*([^\s;]+)",
                  text, flags=re.M)
    if not m:
        raise ValueError(f"{linker}: no FLASH region found")
    return int(m.group(1).strip(), 0), parse_size(m.group(2))


def flash_segments(elf: bytes) -> list[tuple[int, int, int]]:
    """(load address, file offset, size) of every LOAD segment stored in flash."""
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        raise ValueError("not a 32-bit little-endian ELF")
    phoff, = struct.unpack_from("<I", elf, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", elf, 0x2A)
    segs = []
    for i in range(phnum):
        p_type, p_offset, _vaddr, p_paddr, p_filesz = struct.unpack_from(
            "<5I", elf, phoff + i * phentsize)
        in_flash = CHIP_FLASH_START <= p_paddr < CHIP_FLASH_START + 0x08000000
        if p_type == PT_LOAD and p_filesz and in_flash:
            segs.append((p_paddr, p_offset, p_filesz))
    return sorted(segs)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("build_dir", type=Path)
    ap.add_argument("module_dir", type=Path)
    ap.add_argument("flash_addr", type=lambda s: int(s, 0))
    ap.add_argument("--manifest", type=Path)
    args = ap.parse_args()

    module = args.module_dir.as_posix().rstrip("/")
    elf = (args.build_dir / "main.elf").read_bytes()
    binary = (args.build_dir / "main.bin").read_bytes()
    origin, length = linker_flash_region(args.module_dir / "linker.ld")
    budget = min(length, CHIP_FLASH_START + CHIP_FLASH_SIZE - origin)

    failures: list[str] = []

    def check(ok: bool, what: str) -> None:
        print(f"  [{'PASS' if ok else 'FAIL'}] {what}")
        if not ok:
            failures.append(what)

    print(f"=== {module}  (FLASH_ADDR 0x{args.flash_addr:08X}) ===")
    check(origin == args.flash_addr,
          f"linker.ld FLASH ORIGIN 0x{origin:08X} == FLASH_ADDR")

    segs = flash_segments(elf)
    check(bool(segs), "ELF has LOAD segments in flash")
    if segs:
        base = segs[0][0]
        image = bytearray(b"\xff" * (max(a + n for a, _, n in segs) - base))
        for addr, off, size in segs:
            image[addr - base:addr - base + size] = elf[off:off + size]
        check(base == args.flash_addr, f"ELF load address 0x{base:08X} == FLASH_ADDR")
        check(bytes(image) == binary, f"main.bin matches the ELF flash image ({len(image):,} bytes)")

    check(len(binary) <= budget,
          f"size {len(binary):,} B <= budget {budget:,} B "
          f"(linker {length // 1024} KB, chip {CHIP_FLASH_SIZE // 1024} KB)")

    if len(binary) >= 8:
        msp, reset = struct.unpack_from("<II", binary, 0)
        check(SRAM_START < msp <= SRAM_END, f"initial MSP 0x{msp:08X} in SRAM")
        check(bool(reset & 1) and args.flash_addr <= (reset & ~1) < args.flash_addr + len(binary),
              f"reset vector 0x{reset:08X} is Thumb and inside the image")
    else:
        check(False, "image holds a vector table")

    record = {
        "module": module,
        "flash_addr": f"0x{args.flash_addr:08X}",
        "bin_size": len(binary),
        "budget": budget,
        "sha256": hashlib.sha256(binary).hexdigest(),
        "ok": not failures,
    }
    if args.flash_addr == ACTIVE_SLOT_ADDR and len(binary) <= ACTIVE_SLOT_SIZE:
        # Same value tools/seal.py and tools/bl_upload.py send to the bootloader
        padded = binary + b"\xff" * (ACTIVE_SLOT_SIZE - len(binary))
        record["slot_crc32"] = f"0x{zlib.crc32(padded) & 0xFFFFFFFF:08X}"
        print(f"  slot CRC-32 (bootloader tag): {record['slot_crc32']}")

    if args.manifest:
        with args.manifest.open("a") as f:
            f.write(json.dumps(record) + "\n")

    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write(f"| {'✅' if not failures else '❌'} | `{module}` | `{record['flash_addr']}` "
                    f"| {len(binary):,} | {100 * len(binary) / budget:.1f} % "
                    f"| {record.get('slot_crc32', '')} |\n")

    print(f"  -> {'OK' if not failures else f'{len(failures)} check(s) failed'}\n")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
