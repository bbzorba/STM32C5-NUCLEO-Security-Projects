#!/usr/bin/env python3
"""
debug_flash.py — program the bootloader and the application for a debug session,
but only what the board does not already hold.

  python3 tools/debug_flash.py <bootloader.bin> <app.bin>     (make debug-boot)

Reads the flash over SWD (tools/read_flash.py), then:
  - bootloader differs → written at 0x08000000 with STM32_Programmer_CLI
  - application differs, or its CRC tag is wrong → sealed with tools/seal.py,
    so the bootloader verifies and starts it as after a real update
"""

from __future__ import annotations

import sys
import zlib
import struct
import subprocess
from pathlib import Path

from read_flash import REPO, FLASH_BASE, SLOT_ADDR, SLOT_SIZE, TAG_ADDR, read_flash
from bl_upload import find_cube_prog


def main() -> int:
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} <bootloader.bin> <app.bin>")
    boot_bin, app_bin = (Path(a).resolve() for a in sys.argv[1:])
    boot, app = boot_bin.read_bytes(), app_bin.read_bytes()

    dump = read_flash()
    slot = app + b"\xff" * (SLOT_SIZE - len(app))
    crc = zlib.crc32(slot) & 0xFFFFFFFF
    tag, = struct.unpack_from("<I", dump, TAG_ADDR - FLASH_BASE)
    boot_ok = dump[:len(boot)] == boot
    app_ok = dump[SLOT_ADDR - FLASH_BASE:SLOT_ADDR - FLASH_BASE + SLOT_SIZE] == slot and tag == crc

    if boot_ok:
        print(f"Bootloader : up to date ({boot_bin.name})")
    else:
        print(f"Bootloader : programming {boot_bin.name} at 0x{FLASH_BASE:08X}")
        rel = boot_bin.relative_to(REPO).as_posix()      # relative: also valid for a Windows .exe
        subprocess.run([find_cube_prog(), "-c", "port=SWD", "-d", rel, hex(FLASH_BASE)],
                       cwd=REPO, check=True, stdout=subprocess.DEVNULL)

    if app_ok:
        print(f"Application: up to date ({app_bin.name}, CRC tag {tag:08X})")
    else:
        print(f"Application: sealing {app_bin.name} into the active slot")
        subprocess.run([sys.executable, str(REPO / "tools" / "seal.py"), str(app_bin)],
                       cwd=REPO, check=True, stdout=subprocess.DEVNULL)
    return 0


if __name__ == "__main__":
    sys.exit(main())
