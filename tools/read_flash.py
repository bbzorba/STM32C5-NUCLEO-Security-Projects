#!/usr/bin/env python3
"""
read_flash.py — show which bootloader and which application are in flash.

  make read-flash           (USB0 / ST-LINK connected)

Reads the boot region, the active slot and its CRC tag (0x08000000-0x0803DFFF)
over SWD with STM32_Programmer_CLI in hot-plug mode — the MCU is not halted or
reset — and reports for 0x08000000 and the active slot 0x08010000:
  - the name: the "IMGID:<module>" record every build carries (linker/image_id.c)
  - the local build it matches byte for byte, if any (build/<PROJECT_DIR>/*.bin)
  - for the active slot: the CRC tag the bootloader checks before starting it
"""

from __future__ import annotations

import os
import re
import sys
import glob
import zlib
import struct
import subprocess
from pathlib import Path

FLASH_BASE  = 0x08000000
SLOT_ADDR   = 0x08010000           # linker/memory_map.ld
SLOT_SIZE   = 176 * 1024
TAG_ADDR    = 0x0803C000
READ_SIZE   = TAG_ADDR + 0x2000 - FLASH_BASE    # boot region + slot + tag sector
ID_MARKER   = b"IMGID:"
ID_SEARCH   = 0x400                # the record follows the vector table (0x240)
DUMP        = "build/flash_read.bin"

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "tools"))
from bl_upload import find_cube_prog  # noqa: E402


def read_flash() -> bytes:
    cube = find_cube_prog()
    if not cube:
        sys.exit("ERROR: STM32_Programmer_CLI not found (set CUBE_PROG)")
    os.makedirs(REPO / "build", exist_ok=True)
    cmd = [cube, "-c", "port=SWD", "mode=HOTPLUG", "-u", hex(FLASH_BASE), hex(READ_SIZE), DUMP]
    result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        sys.exit("ERROR: reading flash over SWD failed — is USB0 (ST-LINK) connected?\n"
                 + (result.stdout + result.stderr).strip()[-600:])
    return (REPO / DUMP).read_bytes()


def image_name(image: bytes) -> str:
    """Name from the IMGID record, or why there is none."""
    if image[:8] == b"\xff" * 8:
        return "(empty)"
    i = image.find(ID_MARKER, 0, ID_SEARCH)
    if i < 0:
        return "(unknown: no IMGID record — built before it was added)"
    end = image.index(b"\0", i)
    return image[i + len(ID_MARKER):end].decode(errors="replace")


def layout_of(name: str) -> str:
    """LAYOUT from the module's module.mk (standalone when not set)."""
    for mk in glob.glob(str(REPO / "*" / name / "module.mk")):
        m = re.search(r"^LAYOUT\s*:?=\s*(\S+)", Path(mk).read_text(encoding="utf-8"), re.M)
        return m.group(1) if m else "standalone"
    return "?"


def matching_builds(dump: bytes, addr: int) -> list[str]:
    """Local release builds whose .bin is programmed at addr."""
    off = addr - FLASH_BASE
    hits = []
    for path in sorted(glob.glob(str(REPO / "build" / "*" / "*" / "*.bin"))):
        data = Path(path).read_bytes()
        if data and dump[off:off + len(data)] == data:
            hits.append(Path(path).relative_to(REPO).as_posix())
    return hits


def identify(dump: bytes, addr: int) -> tuple[str, list[str]]:
    """Name (IMGID record, else the matching local build) and the matching builds."""
    name = image_name(dump[addr - FLASH_BASE:])
    builds = matching_builds(dump, addr)
    if name.startswith("(unknown") and builds:
        name = f"{Path(builds[0]).stem} (no IMGID record, identified by its local build)"
    return name, builds


def report(title: str, addr: int, name: str, builds: list[str], extra: list[str]) -> None:
    print(f"0x{addr:08X}  {title:<12}: {name}")
    for line in extra:
        print(f"{'':26}{line}")
    print(f"{'':26}{'same as ' + ', '.join(builds) if builds else 'matches no local build in build/'}")


def main() -> int:
    os.chdir(REPO)
    dump = read_flash()
    print("\n=== Flash content (read over SWD, the MCU keeps running) ===")

    boot, boot_builds = identify(dump, FLASH_BASE)
    boot_layout = layout_of(boot.split(" ")[0])
    boots_slot = boot_layout == "bootloader"
    note = {"bootloader": "boot code — starts the active slot",
            "standalone": "standalone image — the active slot is NOT started"}.get(boot_layout, "")
    report("Boot region", FLASH_BASE, boot, boot_builds, [note] if note else [])

    slot = dump[SLOT_ADDR - FLASH_BASE:SLOT_ADDR - FLASH_BASE + SLOT_SIZE]
    tag, = struct.unpack_from("<I", dump, TAG_ADDR - FLASH_BASE)
    crc = zlib.crc32(slot) & 0xFFFFFFFF
    if slot[:8] == b"\xff" * 8:
        status = []
    elif tag == 0xFFFFFFFF:
        status = ["not sealed (no CRC tag) — the bootloader will not start it"]
    elif tag != crc:
        status = [f"CRC tag {tag:08X} != computed {crc:08X} — the bootloader will not start it"]
    else:
        status = [f"CRC tag {tag:08X} OK" + (" — the bootloader starts it" if boots_slot else "")]
    report("Active slot", SLOT_ADDR, *identify(dump, SLOT_ADDR), status)
    return 0


if __name__ == "__main__":
    sys.exit(main())
