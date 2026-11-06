#!/usr/bin/env python3
"""
bl_upload.py — Upload an application through Projects/Bootloader (USART2 / ST-LINK VCP).

What it does:
  1. Opens the serial port and (optionally) resets the board.
  2. Waits for the bootloader's "[BL] READY" line, then sends 'u'.
  3. Sends the image size, waits while the bootloader erases sectors 8-30.
  4. Streams the binary in 256-byte chunks, one ACK per chunk.
  5. Sends the CRC-32 of the image padded to 176 KB with 0xFF — the same
     value tools/seal.py writes — the bootloader checks it and writes the tag.
  6. Prints the bootloader / application output that follows.

Usage:
  python3 tools/bl_upload.py Projects/App_Demo/main.bin [--port PORT] [--reset]
  make bl-upload                       (builds + uploads Projects/App_Demo)
  make bl-upload APP=path/to/app.bin   (uploads a ready binary)

  PORT: COM5 (Windows), /dev/ttyACM0 (Linux). Auto-detected when omitted;
  --fallback-port is used when no ST-LINK is detected (make passes DEFAULT_PORT).

Where it runs:
  Windows       COM port of the ST-LINK VCP.
  Linux         /dev/ttyACM* — your user needs access to it (group 'dialout'
                or 'uucp', depending on the distribution).
  WSL2          The ST-LINK is a Windows device, invisible to WSL2. Unless it
                was attached to WSL with usbipd (then it is /dev/ttyACM*), the
                script re-runs itself with the Windows Python (python.exe).

Reset (--reset), first available of:
  STM32_Programmer_CLI ($CUBE_PROG, PATH, STM32CubeProgrammer / STM32CubeIDE
  install) → st-flash (stlink-tools) → ask you to press RESET.
  Without --reset: press RESET yourself, or hold B1 while resetting — the
  bootloader then stays and repeats "[BL] READY" until the upload starts.

Requires pyserial:
  Debian/Ubuntu : sudo apt install python3-serial
  Other Linux   : python3 -m venv .venv && .venv/bin/pip install pyserial
  Windows       : py -m pip install pyserial
"""

import os
import sys
import glob
import time
import zlib
import shutil
import struct
import argparse
import platform
import subprocess
from pathlib import Path

# ── Configuration (must match Projects/Bootloader/inc/bootloader.h) ──────────
BAUD             = 115200
ACTIVE_SLOT_SIZE = 22 * 8192       # 176 KB (sectors 8-29)
CHUNK_SIZE       = 256
CMD_UPDATE       = b"u"
ACK              = 0x79
NACK             = 0x1F
READY_MARKER     = b"[BL] READY"
ERASE_TIMEOUT_S  = 15.0            # 23 sectors erased before the first ACK
ACK_TIMEOUT_S    = 3.0
ST_VID           = 0x0483          # STMicroelectronics USB vendor ID
# ST-LINK VCP product IDs (V2-1, V3, V3E, V3SET, V3MINIE, V3PWR …). 0x5740 is
# NOT here: that is an STM32's own USB CDC port (another board / application).
STLINK_PIDS      = {0x374B, 0x374E, 0x374F, 0x3752, 0x3753, 0x3754, 0x3755, 0x3757}
# ─────────────────────────────────────────────────────────────────────────────

IS_WINDOWS = os.name == "nt"
IS_WSL     = not IS_WINDOWS and ("microsoft" in platform.release().lower()
                                 or "WSL_DISTRO_NAME" in os.environ)

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


# ── Platform helpers ─────────────────────────────────────────────────────────

def need_pyserial() -> None:
    if serial is not None:
        return
    print("ERROR: pyserial is not installed for", sys.executable)
    if IS_WINDOWS:
        print("  Install it with: py -m pip install pyserial")
    else:
        print("  Debian/Ubuntu : sudo apt install python3-serial")
        print("  Other distros : python3 -m venv .venv && .venv/bin/pip install pyserial")
        print("                  then run the script with .venv/bin/python")
    sys.exit(1)


def st_ports() -> list[str]:
    """Serial ports of an ST-LINK virtual COM port (VID/PID, else by name)."""
    found = []
    for p in serial.tools.list_ports.comports():
        text = f"{p.description} {p.manufacturer or ''} {p.product or ''}".lower()
        if (p.vid == ST_VID and p.pid in STLINK_PIDS) or "stlink" in text or "st-link" in text:
            found.append(p.device)
    return found


def find_cube_prog() -> str | None:
    """Locate STM32_Programmer_CLI: $CUBE_PROG, PATH, standard installs."""
    env = os.environ.get("CUBE_PROG")
    if env and Path(env).is_file():
        return env
    for name in ("STM32_Programmer_CLI", "STM32_Programmer_CLI.exe"):
        hit = shutil.which(name)
        if hit:
            return hit
    if IS_WINDOWS:
        patterns = [r"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe",
                    r"C:\ST\STM32CubeIDE*\STM32CubeIDE\plugins\*cubeprogrammer*\tools\bin\STM32_Programmer_CLI.exe"]
    elif IS_WSL:
        patterns = ["/mnt/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
                    "/mnt/c/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI.exe"]
    else:
        patterns = [os.path.expanduser("~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI"),
                    "/opt/st/stm32cubeide*/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI"]
    for pat in patterns:
        hits = sorted(glob.glob(pat))
        if hits:
            return hits[-1]
    return None


def reset_board() -> bool:
    """Hardware reset over SWD. Returns False when no tool could do it."""
    cube = find_cube_prog()
    if cube:
        cmds = [[cube, "-c", "port=SWD", "freq=4000", "-rst"]]
    elif shutil.which("st-flash"):
        cmds = [["st-flash", "reset"]]
    else:
        print("  No STM32_Programmer_CLI or st-flash found.")
        return False
    for cmd in cmds:
        print("  $", " ".join(f'"{a}"' if " " in a else a for a in cmd))
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            return True
        print((result.stdout + result.stderr).strip()[-600:])
    print("  Reset over SWD failed.")
    return False


def rerun_with_windows_python() -> int:
    """WSL2 without a usbipd-attached ST-LINK: hand over to python.exe."""
    py = shutil.which("python.exe") or shutil.which("py.exe")
    if not py:
        print("ERROR: no ST-LINK serial port in WSL and no Windows python.exe found.")
        print("  Install Python on Windows (with pyserial), or attach the ST-LINK")
        print("  to WSL with usbipd-win so it shows up as /dev/ttyACM0.")
        return 1

    def winpath(p: str) -> str:
        return subprocess.run(["wslpath", "-w", p], capture_output=True,
                              text=True, check=True).stdout.strip()

    args = sys.argv[1:]
    # The binary is the only positional argument — translate it
    out, i = [], 0
    while i < len(args):
        a = args[i]
        if a in ("--port", "--fallback-port", "--wait"):
            out += [a, args[i + 1]]
            i += 2
            continue
        out.append(a if a.startswith("-") else winpath(os.path.abspath(a)))
        i += 1

    env = dict(os.environ)
    cube = find_cube_prog()
    if cube:
        env["CUBE_PROG"] = winpath(cube)
    env["PYTHONIOENCODING"] = "utf-8"          # keep "—" etc. readable in this terminal
    env["WSLENV"] = ":".join(filter(None, [env.get("WSLENV"), "PYTHONIOENCODING"]
                                  + (["CUBE_PROG"] if cube else [])))

    print("WSL2: ST-LINK serial port is only visible to Windows — "
          "re-running with the Windows Python.\n", flush=True)
    cmd = [py, winpath(os.path.abspath(__file__))] + out
    return subprocess.run(cmd, env=env).returncode


# ── Protocol ─────────────────────────────────────────────────────────────────

def wait_ready(ser, timeout_s: float) -> None:
    """Echo the bootloader banner until the READY marker line arrives."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        line = ser.readline()
        if not line:
            continue
        print("   ", line.decode(errors="replace").rstrip())
        if READY_MARKER in line:
            return
    raise RuntimeError("bootloader READY not seen — reset the board, hold B1 "
                       "while resetting, or use --reset")


def expect_ack(ser, timeout_s: float, step: str) -> None:
    ser.timeout = timeout_s
    resp = ser.read(1)
    if not resp:
        raise RuntimeError(f"{step}: no response (timeout)")
    if resp[0] == NACK:
        raise RuntimeError(f"{step}: NACK from bootloader")
    if resp[0] != ACK:
        raise RuntimeError(f"{step}: unexpected byte 0x{resp[0]:02X}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Upload an image via Projects/Bootloader")
    ap.add_argument("binary", help="application .bin linked at 0x08010000")
    ap.add_argument("--port", help="serial port, e.g. COM5 or /dev/ttyACM0 "
                                   "(default: auto-detect the ST-LINK VCP)")
    ap.add_argument("--fallback-port", help="port to use when auto-detection finds no ST-LINK")
    ap.add_argument("--reset", action="store_true", help="reset the board over SWD first")
    ap.add_argument("--wait", type=float, default=30.0, help="seconds to wait for READY")
    args = ap.parse_args()

    # WSL2: COMx ports and the ST-LINK itself live on the Windows side
    if IS_WSL:
        if args.port and args.port.upper().startswith("COM"):
            return rerun_with_windows_python()
        linux_fallback = (args.fallback_port or "").startswith("/dev/") \
            and os.path.exists(args.fallback_port)
        if not args.port and not linux_fallback and (serial is None or not st_ports()):
            return rerun_with_windows_python()

    need_pyserial()

    bin_path = Path(args.binary)
    if not bin_path.exists():
        print(f"ERROR: binary not found: {bin_path}")
        return 1

    binary = bin_path.read_bytes()
    if not binary or len(binary) > ACTIVE_SLOT_SIZE:
        print(f"ERROR: binary size {len(binary)} bytes must be 1..{ACTIVE_SLOT_SIZE} "
              f"({ACTIVE_SLOT_SIZE // 1024} KB)")
        return 1

    port = args.port
    if not port:
        ports = st_ports()
        if ports:
            port = ports[0]
        elif args.fallback_port:
            port = args.fallback_port
            print(f"No ST-LINK serial port detected — using the default port {port}")
        else:
            print("ERROR: no ST-LINK serial port found — is the board connected? "
                  "Otherwise pass --port")
            return 1

    # Same CRC as seal.py: CRC-32 over the image padded to the full slot
    padded    = binary + b"\xff" * (ACTIVE_SLOT_SIZE - len(binary))
    crc_value = zlib.crc32(padded) & 0xFFFFFFFF

    print(f"\n=== Bootloader upload: {bin_path.name} ===")
    print(f"  Port        : {port} @ {BAUD}")
    print(f"  Binary size : {len(binary):,} bytes  ({len(binary) / 1024:.1f} KB)")
    print(f"  CRC-32      : 0x{crc_value:08X} (padded to {ACTIVE_SLOT_SIZE // 1024} KB)")

    try:
        ser = serial.Serial(port, BAUD, timeout=0.2)
    except serial.SerialException as e:
        msg = f"cannot open {port}: {e}"
        if not IS_WINDOWS and "ermission" in str(e):
            msg += ("\n  Add yourself to the serial group and log in again: "
                    "sudo usermod -aG dialout $USER")
        raise RuntimeError(msg)

    with ser:
        ser.reset_input_buffer()
        if args.reset:
            print("\n[0/4] Resetting board ...")
            if not reset_board():
                print(f"  Press RESET on the board now (waiting up to {args.wait:.0f} s) ...")
        else:
            print(f"\nReset the board now, or hold B1 while resetting "
                  f"(waiting up to {args.wait:.0f} s) ...")

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
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as e:
        print(f"\nERROR: {e}")
        sys.exit(1)
