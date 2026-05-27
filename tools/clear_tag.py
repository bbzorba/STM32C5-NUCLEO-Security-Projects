#!/usr/bin/env python3
"""
clear_tag.py — Erase the CRC tag at sector 30 (0x0803C000).

When 'make flash' writes a new binary to the active slot (0x08010000) without
running seal.py, the old CRC tag from a previous seal remains stale.  Secure_Boot
detects a mismatch and halts ("BOOT FAILED").  This script writes 0xFFFFFFFF to
the 4-byte tag word so the bootloader sees a blank (unsealed) slot and skips it
rather than boot-looping on a bad CRC.

Call 'make seal-active' after testing to properly seal the slot.
"""

import subprocess
import tempfile
import os
import sys

CUBE = r"C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
CRC_TAG_ADDR = 0x0803C000   # sector 30 base address

with tempfile.NamedTemporaryFile(suffix="_clear_tag.bin", delete=False) as f:
    f.write(b"\xff\xff\xff\xff")
    tmp = f.name

try:
    result = subprocess.run(
        [CUBE, "-c", "port=SWD", "freq=4000",
         "-d", tmp, hex(CRC_TAG_ADDR), "-rst"],
        capture_output=True, text=True
    )
    if result.returncode == 0:
        print("  CRC tag cleared (sector 30 blank — use 'make seal-active' to seal).")
    else:
        print(result.stdout, file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        sys.exit(result.returncode)
finally:
    os.remove(tmp)
