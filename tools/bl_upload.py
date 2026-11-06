#!/usr/bin/env python3
"""
bl_upload.py — Upload an application through Projects/Bootloader (USART2 / ST-LINK VCP).

What it does:
  1. Opens the serial port and (optionally) resets the board over SWD.
  2. Waits for the bootloader's "[BL] READY" line, then sends 'u'.
  3. Sends the image size, waits while the bootloader erases sectors 8-30.
  4. Streams the binary in 256-byte chunks, one ACK per chunk.
  5. Sends the CRC-32 of the image padded to 176 KB with 0xFF — the same
     value tools/seal.py writes — the bootloader checks it and writes the tag.
  6. Prints the bootloader / application output that follows.

Usage:
  python tools/bl_upload.py Projects/App_Demo/main.bin [--port COM5] [--reset]

Requires: pip install pyserial
"""

import os
import sys
import time
import zlib
import struct
import argparse
import subprocess
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed — run: pip install pyserial")
    sys.exit(1)

# ── Configuration (must match Projects/Bootloader/inc/bootloader.h) ──────────
# STM32_Programmer_CLI path; `make` exports CUBE_PROG (auto-detected, e.g. the
# copy bundled with STM32CubeIDE). Falls back to the standalone install.
CUBE_PROG = os.environ.get("CUBE_PROG") or (
    r"C:\Program Files\STMicroelectronics\STM32Cube"
    r"\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
)
BAUD             = 115200
ACTIVE_SLOT_SIZE = 22 * 8192       # 176 KB (sectors 8-29)
CHUNK_SIZE       = 256
CMD_UPDATE       = b"u"
ACK              = 0x79
NACK             = 0x1F
READY_MARKER     = b"[BL] READY"
ERASE_TIMEOUT_S  = 15.0            # 23 sectors erased before the first ACK
ACK_TIMEOUT_S    = 3.0
# ─────────────────────────────────────────────────────────────────────────────


def find_port() -> str:
    """Pick the ST-LINK virtual COM port (same idea as tools/monitor.ps1)."""
    for p in serial.tools.list_ports.comports():
        text = f"{p.description} {p.manufacturer or ''}".lower()
        if "stlink" in text or "st-link" in text or "stmicro" in text:
            return p.device
    print("ERROR: no ST-LINK VCP found — pass --port COMx")
    sys.exit(1)


def reset_board() -> None:
    """Hardware reset over SWD so the bootloader opens its update window."""
    cmd = [CUBE_PROG, "-c", "port=SWD", "freq=4000", "-rst"]
    print("  $", " ".join(f'"{a}"' if " " in a else a for a in cmd))
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout, result.stderr)
        raise RuntimeError(f"STM32CubeProgrammer failed (exit {result.returncode})")


def wait_ready(ser: serial.Serial, timeout_s: float) -> None:
    """Echo the bootloader banner until the READY marker line arrives."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        line = ser.readline()
        if not line:
            continue
        print("   ", line.decode(errors="replace").rstrip())
        if READY_MARKER in line:
            return
    raise RuntimeError("bootloader READY not seen — reset the board (or use --reset)")


def expect_ack(ser: serial.Serial, timeout_s: float, step: str) -> None:
    ser.timeout = timeout_s
    resp = ser.read(1)
    if not resp:
        raise RuntimeError(f"{step}: no response (timeout)")
    if resp[0] == NACK:
        raise RuntimeError(f"{step}: NACK from bootloader")
    if resp[0] != ACK:
        raise RuntimeError(f"{step}: unexpected byte 0x{resp[0]:02X}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Upload an image via Projects/Bootloader")
    ap.add_argument("binary", help="application .bin linked at 0x08010000")
    ap.add_argument("--port", help="serial port (default: auto-detect ST-LINK VCP)")
    ap.add_argument("--reset", action="store_true", help="reset the board over SWD first")
    ap.add_argument("--wait", type=float, default=30.0, help="seconds to wait for READY")
    args = ap.parse_args()

    bin_path = Path(args.binary)
    if not bin_path.exists():
        print(f"ERROR: binary not found: {bin_path}")
        sys.exit(1)

    binary = bin_path.read_bytes()
    if not binary or len(binary) > ACTIVE_SLOT_SIZE:
        print(f"ERROR: binary size {len(binary)} bytes must be 1..{ACTIVE_SLOT_SIZE} "
              f"({ACTIVE_SLOT_SIZE // 1024} KB)")
        sys.exit(1)

    # Same CRC as seal.py: CRC-32 over the image padded to the full slot
    padded    = binary + b"\xff" * (ACTIVE_SLOT_SIZE - len(binary))
    crc_value = zlib.crc32(padded) & 0xFFFFFFFF
    port      = args.port or find_port()

    print(f"\n=== Bootloader upload: {bin_path.name} ===")
    print(f"  Port        : {port} @ {BAUD}")
    print(f"  Binary size : {len(binary):,} bytes  ({len(binary) / 1024:.1f} KB)")
    print(f"  CRC-32      : 0x{crc_value:08X} (padded to {ACTIVE_SLOT_SIZE // 1024} KB)")

    with serial.Serial(port, BAUD, timeout=0.2) as ser:
        ser.reset_input_buffer()
        if args.reset:
            print("\n[0/4] Resetting board ...")
            reset_board()
        else:
            print(f"\nReset the board now (waiting up to {args.wait:.0f} s) ...")

        wait_ready(ser, args.wait)
        ser.reset_input_buffer()

        print("\n[1/4] Requesting update ...")
        ser.write(CMD_UPDATE)
        expect_ack(ser, ACK_TIMEOUT_S, "update request")

        print("[2/4] Sending size, erasing active slot ...")
        ser.write(struct.pack("<I", len(binary)))
        expect_ack(ser, ERASE_TIMEOUT_S, "erase")

        print("[3/4] Programming ...")
        for off in range(0, len(binary), CHUNK_SIZE):
            ser.write(binary[off:off + CHUNK_SIZE])
            expect_ack(ser, ACK_TIMEOUT_S, f"chunk @ +0x{off:05X}")
            done = min(off + CHUNK_SIZE, len(binary))
            print(f"\r      {done:>7,} / {len(binary):,} bytes", end="", flush=True)
        print()

        print("[4/4] Verifying CRC + sealing ...")
        ser.write(struct.pack("<I", crc_value))
        expect_ack(ser, ERASE_TIMEOUT_S, "CRC verify")

        print("\n=== UPLOADED ===  Board output:\n")
        ser.timeout = 0.2
        end = time.monotonic() + 3.0
        while time.monotonic() < end:
            line = ser.readline()
            if line:
                print("   ", line.decode(errors="replace").rstrip())


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as e:
        print(f"\nERROR: {e}")
        sys.exit(1)
