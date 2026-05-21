#!/usr/bin/env python3
"""
seal.py — Seal the Secure Boot active application slot on STM32C562RE.

What it does:
  1. Reads the application binary and pads it to the full 176 KB slot size
     with 0xFF (erased flash byte value).
  2. Computes CRC-32 over the padded 176 KB — same algorithm as the STM32
     hardware CRC peripheral (IEEE 802.3 / zlib, poly=0x04C11DB7, XorOut=0xFFFFFFFF).
  3. Flashes the padded binary at 0x08010000 (sectors 8-29, active slot).
     CubeProgrammer erases only those sectors — the bootloader at 0x08000000
     is never touched.
  4. Writes the 4-byte CRC tag (little-endian) to 0x0803C000 (sector 30).
     CubeProgrammer erases sector 30 before writing.
  5. Resets the board — the bootloader now finds a valid tag and jumps to
     the application.

Usage:
  python tools/seal.py Projects/App_Demo/main.bin
"""

import sys
import zlib
import struct
import tempfile
import subprocess
from pathlib import Path

# ── Configuration ─────────────────────────────────────────────────────────────
CUBE_PROG = (
    r"C:\Program Files\STMicroelectronics\STM32Cube"
    r"\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
)
SWD_CONNECT = ["-c", "port=SWD", "freq=4000"]

ACTIVE_ADDR      = 0x08010000
ACTIVE_SLOT_SIZE = 22 * 8192       # 176 KB (sectors 8-29)
TAG_ADDR         = 0x0803C000      # sector 30 — CRC tag for active slot
# ─────────────────────────────────────────────────────────────────────────────


def cube(args: list[str], *, reset_after: bool = False) -> None:
    """Run CubeProgrammer CLI and raise RuntimeError on failure."""
    cmd = [CUBE_PROG] + SWD_CONNECT + args
    if reset_after:
        cmd += ["-rst"]
    print("  $", " ".join(f'"{a}"' if " " in a else a for a in cmd))
    result = subprocess.run(cmd, capture_output=True, text=True)
    # Show relevant lines (avoid flooding with progress bars)
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith("---"):
            print("   ", stripped)
    if result.returncode != 0:
        print(result.stderr)
        raise RuntimeError(f"STM32CubeProgrammer failed (exit {result.returncode})")


def main() -> None:
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <app_binary.bin>")
        sys.exit(1)

    bin_path = Path(sys.argv[1])
    if not bin_path.exists():
        print(f"ERROR: binary not found: {bin_path}")
        sys.exit(1)

    binary = bin_path.read_bytes()
    if len(binary) > ACTIVE_SLOT_SIZE:
        print(f"ERROR: binary size {len(binary)} bytes "
              f"exceeds active slot size {ACTIVE_SLOT_SIZE} bytes ({ACTIVE_SLOT_SIZE // 1024} KB)")
        sys.exit(1)

    # ── Step 1: Compute CRC over the full padded slot ─────────────────────────
    padded = binary + b'\xff' * (ACTIVE_SLOT_SIZE - len(binary))
    crc_value = zlib.crc32(padded) & 0xFFFFFFFF

    print(f"\n=== Sealing active slot: {bin_path.name} ===")
    print(f"  Binary size : {len(binary):,} bytes  ({len(binary) / 1024:.1f} KB)")
    print(f"  Padded to   : {ACTIVE_SLOT_SIZE:,} bytes ({ACTIVE_SLOT_SIZE // 1024} KB) with 0xFF")
    print(f"  CRC-32      : 0x{crc_value:08X}")

    # ── Step 2: Flash the padded binary at 0x08010000 ────────────────────────
    # Writing a full 176 KB file forces CubeProgrammer to erase all of
    # sectors 8-29 before writing — guarantees a clean, predictable slot.
    padded_path = Path(tempfile.mktemp(suffix="_app_padded.bin"))
    tag_path    = Path(tempfile.mktemp(suffix="_crc_tag.bin"))
    try:
        padded_path.write_bytes(padded)
        # Little-endian 32-bit value — matches uint32_t read on Cortex-M33 (LE)
        tag_path.write_bytes(struct.pack('<I', crc_value))

        print(f"\n[1/2] Flashing {ACTIVE_SLOT_SIZE // 1024} KB at "
              f"0x{ACTIVE_ADDR:08X} (erases sectors 8-29 only)...")
        cube(["-d", str(padded_path), f"0x{ACTIVE_ADDR:08X}"])

        # ── Step 3: Write CRC tag to sector 30 ───────────────────────────────
        print(f"\n[2/2] Writing CRC tag 0x{crc_value:08X} "
              f"to 0x{TAG_ADDR:08X} (sector 30)...")
        cube(["-d", str(tag_path), f"0x{TAG_ADDR:08X}"], reset_after=True)

    finally:
        padded_path.unlink(missing_ok=True)
        tag_path.unlink(missing_ok=True)

    print(f"\n=== SEALED ===")
    print(f"  Active slot  0x{ACTIVE_ADDR:08X}  CRC tag: 0x{crc_value:08X}")
    print(f"  Board reset — bootloader will now verify and jump to your application.")
    print(f"\nExpected bootloader output:")
    print(f"  CRC tag @ 0x0803C000 : {crc_value:08X}")
    print(f"  Computed CRC         : {crc_value:08X}")
    print(f"  CRC  : OK")
    print(f"  MSP  : valid — jumping to application")


if __name__ == "__main__":
    main()
