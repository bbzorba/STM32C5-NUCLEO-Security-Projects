#!/usr/bin/env python3
"""
bl_upload.py — Upload an application through Projects/Bootloader.

Links (same protocol on both, picked with --link, default auto):
  uart  USART2 through the ST-LINK virtual COM port (ST-LINK USB cable)
  usb   USB CDC ACM of the bootloader itself, USB-C user connector
        (VID:PID CAFE:B0C5, "C562 Bootloader CDC")
  auto  usb when the bootloader's or the application's USB port is present,
        else uart

USB while the application runs: an application with its own USB port
(CAFE:B0C6, e.g. Projects/App_Demo) is sent a 1200-baud touch — open at
1200 baud, close — and restarts into the bootloader, which then stays until
the upload arrives. No SWD and no button needed.

What it does:
  1. Opens the serial port and (optionally) resets the board.
  2. Waits for the bootloader's "[BL] READY" line, then sends 'u'.
  3. Sends the image size, waits while the bootloader erases sectors 8-30.
  4. Streams the binary in 256-byte chunks, one ACK per chunk.
  5. Sends the CRC-32 of the image padded to 176 KB with 0xFF — the same
     value tools/seal.py writes — the bootloader checks it and writes the tag.
  6. Prints the bootloader / application output that follows.

Usage:
  python3 tools/bl_upload.py build/Projects/App_Demo/App_Demo.bin [--port PORT] [--reset]
  make bl-upload                       (builds + uploads Projects/App_Demo)
  make bl-upload LINK=usb              (force the USB-C link)
  make bl-upload APP=path/to/app.bin   (uploads a ready binary)

  PORT: COM5 (Windows), /dev/ttyACM0 (Linux). Auto-detected when omitted;
  --fallback-port is used when no ST-LINK is detected (make passes DEFAULT_PORT).

Where it runs:
  Windows       COM port of the ST-LINK VCP.
  Linux         /dev/ttyACM* — your user needs access to it (group 'dialout'
                or 'uucp', depending on the distribution).
  WSL2          USB devices belong to Windows. A device attached to WSL with
                usbipd shows up as /dev/ttyACM*; otherwise the script re-runs
                itself with the Windows Python (python.exe). With
                --wsl-usbipd (make bl-upload USBIPD=1) it always stays in WSL.

Reset (--reset, used when no application USB port can be touched), first of:
  STM32_Programmer_CLI ($CUBE_PROG, PATH, STM32CubeProgrammer / STM32CubeIDE
  install) → st-flash (stlink-tools) → ask you to press RESET.
  Without --reset: press RESET yourself, or hold B1 while resetting — the
  bootloader then stays and repeats "[BL] READY" until the upload starts.

Requires pyserial:
  Debian/Ubuntu : sudo apt install python3-serial
  Other Linux   : python3 -m venv .venv && .venv/bin/pip install pyserial
  Windows       : py -m pip install pyserial
"""

from __future__ import annotations

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
BL_USB_VID       = 0xCAFE          # Drivers/USB_CDC/inc/usb_cdc.h
BL_USB_PID       = 0xB0C5          # USB_CDC_PID: the bootloader
APP_USB_PID      = 0xB0C6          # USB_CDC_APP_PID: an application (1200-baud touch)
TOUCH_BAUD       = 1200            # USB_CDC_TOUCH_BAUD
USB_AFTER_RESET_S = 10.0           # bootloader USB port must appear this soon after an SWD reset
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


def bl_usb_ports() -> list[str]:
    """Serial ports of the bootloader's own USB CDC (USB-C user connector)."""
    return [p.device for p in serial.tools.list_ports.comports()
            if p.vid == BL_USB_VID and p.pid == BL_USB_PID]


def app_usb_ports() -> list[str]:
    """Serial ports of an application's USB CDC that accepts the 1200-baud touch."""
    return [p.device for p in serial.tools.list_ports.comports()
            if p.vid == BL_USB_VID and p.pid == APP_USB_PID]


def touch_into_bootloader(port: str) -> None:
    """1200-baud touch: the application resets into the bootloader, which stays."""
    print(f"  1200-baud touch on {port} — the application restarts into the bootloader")
    try:
        with serial.Serial(port, TOUCH_BAUD) as ser:
            time.sleep(0.1)
            ser.dtr = False            # port "closed" at 1200 baud → bootloader request
    except (serial.SerialException, OSError):
        pass                           # the device may vanish while the port closes


def port_exists(port: str) -> bool:
    """True when the port is present on this machine (COMx listed, or the /dev node exists)."""
    if port.startswith("/dev/"):
        return os.path.exists(port)
    return any(p.device.upper() == port.upper() for p in serial.tools.list_ports.comports())


def wait_for_port(find, timeout_s: float) -> str | None:
    """Poll until find() returns a port (USB re-enumerates after a reset)."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        ports = find()
        if ports:
            time.sleep(0.3)            # let the OS finish setting the port up
            return ports[0]
        time.sleep(0.2)
    return None


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
        result = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
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
        if a in ("--port", "--fallback-port", "--wait", "--link"):
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

    print("WSL2: the board's serial ports are only visible to Windows — "
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


def enter_bootloader(wait_s: float) -> int:
    """--enter: touch the application's USB port, wait for the bootloader's."""
    if bl_usb_ports():
        print(f"Bootloader already running: {bl_usb_ports()[0]}")
        return 0
    apps = app_usb_ports()
    if not apps:
        print("ERROR: no application USB port (CAFE:B0C6) found — is USB1 connected and does the "
              "running application support the 1200-baud touch (App_Demo)? Otherwise hold B1 "
              "and press RESET.")
        return 1
    touch_into_bootloader(apps[0])
    port = wait_for_port(bl_usb_ports, wait_s)
    if not port:
        print("ERROR: the bootloader's USB port (CAFE:B0C5) did not appear")
        return 1
    print(f"Bootloader is waiting on {port} (LD1 blinks) — it stays until an upload arrives "
          "or the board is reset.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Upload an image via Projects/Bootloader")
    ap.add_argument("binary", nargs="?", help="application .bin linked at 0x08010000")
    ap.add_argument("--enter", action="store_true",
                    help="only put the board into the bootloader (1200-baud touch on the "
                         "application's USB port) and leave it waiting — no upload")
    ap.add_argument("--port", help="serial port, e.g. COM5 or /dev/ttyACM0 "
                                   "(default: auto-detect the bootloader USB port or the ST-LINK VCP)")
    ap.add_argument("--link", choices=("auto", "uart", "usb"), default="auto",
                    help="uart = ST-LINK VCP, usb = bootloader USB CDC (USB-C user port)")
    ap.add_argument("--fallback-port", help="UART port to use when no ST-LINK is detected")
    ap.add_argument("--wsl-usbipd", action="store_true",
                    help="WSL: the board's USB devices are attached to WSL with usbipd — "
                         "stay in WSL and wait for /dev/ttyACM* instead of using Windows")
    ap.add_argument("--reset", action="store_true", help="reset the board over SWD first")
    ap.add_argument("--wait", type=float, default=30.0, help="seconds to wait for READY")
    args = ap.parse_args()

    # WSL2: COMx ports (and devices not attached with usbipd) live on Windows
    if IS_WSL:
        if args.port and args.port.upper().startswith("COM"):
            return rerun_with_windows_python()
        if not args.port:
            linux_fallback = (args.fallback_port or "").startswith("/dev/") \
                and os.path.exists(args.fallback_port)
            local = []
            if serial is not None:
                if args.link in ("auto", "usb"):
                    local += bl_usb_ports() + app_usb_ports()
                if args.link in ("auto", "uart"):
                    local += st_ports()
            if not local and not args.wsl_usbipd and not (args.link != "usb" and linux_fallback):
                return rerun_with_windows_python()

    need_pyserial()

    if args.enter:
        return enter_bootloader(args.wait)
    if not args.binary:
        ap.error("the binary is required (or use --enter)")

    bin_path = Path(args.binary)
    if not bin_path.exists():
        print(f"ERROR: binary not found: {bin_path}")
        return 1

    binary = bin_path.read_bytes()
    if not binary or len(binary) > ACTIVE_SLOT_SIZE:
        print(f"ERROR: binary size {len(binary)} bytes must be 1..{ACTIVE_SLOT_SIZE} "
              f"({ACTIVE_SLOT_SIZE // 1024} KB)")
        return 1

    # Pick the link + port
    link, port = args.link, args.port
    if link == "auto":
        link = "usb" if (not port and (bl_usb_ports() or app_usb_ports())) else "uart"
    if not port and link == "uart":
        ports = st_ports()
        if ports:
            port = ports[0]
        elif args.fallback_port and port_exists(args.fallback_port):
            port = args.fallback_port
            print(f"No ST-LINK serial port detected — trying the default port {port}")
        else:
            print("ERROR: UART link selected, but this PC has no ST-LINK virtual COM port — is "
                  "USB0 (the ST-LINK cable) connected to this PC?"
                  + (f" The default port {args.fallback_port} does not exist either."
                     if args.fallback_port else "")
                  + " To upload over USB1 use LINK=usb.")
            return 1

    # Same CRC as seal.py: CRC-32 over the image padded to the full slot
    padded    = binary + b"\xff" * (ACTIVE_SLOT_SIZE - len(binary))
    crc_value = zlib.crc32(padded) & 0xFFFFFFFF

    # USB: the CDC port only exists while the bootloader runs — reset first,
    # then wait for it to (re-)enumerate
    if link == "usb" and not port:
        wait_s = args.wait
        if not bl_usb_ports() and app_usb_ports():
            print("\n[0/4] Application is running ...")
            touch_into_bootloader(app_usb_ports()[0])
        elif not bl_usb_ports():
            print("\nNo USB port of the board on this PC (bootloader CAFE:B0C5 / application "
                  "CAFE:B0C6) — the running application may have none.")
            if args.reset:
                print("[0/4] Resetting board over SWD so the bootloader's USB port appears ...")
                if reset_board():
                    wait_s = min(wait_s, USB_AFTER_RESET_S)   # it enumerates within ~1 s
                else:
                    print("  Press RESET on the board now (hold B1 to stay in the bootloader) ...")
            else:
                print("Reset the board now (hold B1 to stay in the bootloader) ...")
        port = wait_for_port(bl_usb_ports, wait_s)
        if not port:
            raise RuntimeError(f"USB link selected, but the bootloader's USB port (CAFE:B0C5) did "
                               f"not appear within {wait_s:.0f} s — is USB1 (the USB-C user port) "
                               f"connected to this PC? (WSL: is usbipd attach running?) "
                               f"To upload over the ST-LINK use LINK=uart.")
        args.reset = False             # already done

    print(f"\n=== Bootloader upload: {bin_path.name} ===")
    print(f"  Link        : {'USB CDC (USB-C user port)' if link == 'usb' else 'UART (ST-LINK VCP)'}")
    print(f"  Port        : {port}" + ("" if link == "usb" else f" @ {BAUD}"))
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
        # UART: drop stale bytes. USB: keep them — opening the port is what makes
        # the bootloader send "[BL] READY", flushing here would discard it.
        if link != "usb":
            ser.reset_input_buffer()
        if args.reset:
            print("\n[0/4] Resetting board ...")
            if not reset_board():
                print(f"  Press RESET on the board now (waiting up to {args.wait:.0f} s) ...")
        elif link == "usb":
            print("\nUSB port open — waiting for the bootloader ...")
        else:
            print(f"\nReset the board now, or hold B1 while resetting "
                  f"(waiting up to {args.wait:.0f} s) ...")

        wait_ready(ser, args.wait)
        ser.reset_input_buffer()

        try:
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

        except RuntimeError:
            # Show why the bootloader gave up ("[BL] Update : TIMEOUT" …)
            ser.timeout = 0.2
            end = time.monotonic() + 1.5
            print("\n    Board output after the error:")
            try:
                while time.monotonic() < end:
                    line = ser.readline()
                    if line:
                        print("     ", line.decode(errors="replace").rstrip())
            except (serial.SerialException, OSError):
                pass
            raise

        print("\n=== UPLOADED ===  Board output:\n")
        ser.timeout = 0.2
        end = time.monotonic() + 3.0
        try:
            while time.monotonic() < end:
                line = ser.readline()
                if line:
                    print("   ", line.decode(errors="replace").rstrip())
        except (serial.SerialException, OSError):
            # USB link: the bootloader detaches USB when it jumps to the application
            print("    (USB port closed — bootloader jumped to the application)")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as e:
        print(f"\nERROR: {e}")
        sys.exit(1)
