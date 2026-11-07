#!/usr/bin/env python3
"""
tools/bl_upload.py against a simulated Projects/Bootloader (no hardware).

A pseudo-terminal stands in for the serial port: the test plays the
bootloader on the master side (READY, ACK/NACK, CRC check — protocol in
Projects/Bootloader/ARCHITECTURE.md), and runs bl_upload.py unchanged as a
subprocess on the slave side. Linux / macOS / WSL only (needs pty).

  python3 -m unittest discover -s tools/tests -v
"""

from __future__ import annotations

import os
import pty
import sys
import tty
import zlib
import select
import struct
import tempfile
import threading
import subprocess
import unittest
from pathlib import Path

TOOL       = Path(__file__).resolve().parents[1] / "bl_upload.py"
SLOT_SIZE  = 176 * 1024
CHUNK_SIZE = 256
ACK, NACK  = 0x79, 0x1F


class FakeBootloader(threading.Thread):
    """Bootloader side of the update protocol on a pty master."""

    def __init__(self, master: int, corrupt_crc: bool = False):
        super().__init__(daemon=True)
        self.master = master
        self.corrupt_crc = corrupt_crc
        self.image = bytearray()
        self.result: str | None = None
        self._buf = bytearray()

    def _read(self, n: int, timeout: float = 5.0) -> bytes:
        while len(self._buf) < n:
            ready, _, _ = select.select([self.master], [], [], timeout)
            if not ready:
                raise TimeoutError(f"host sent {len(self._buf)} of {n} bytes")
            self._buf += os.read(self.master, 4096)
        data, self._buf = bytes(self._buf[:n]), self._buf[n:]
        return data

    def _send(self, data: bytes) -> None:
        os.write(self.master, data)

    def run(self) -> None:
        try:
            self._serve()
        except (OSError, TimeoutError) as e:
            self.result = f"error: {e}"

    def _serve(self) -> None:
        # Stay mode: READY until 'u' arrives (the host may flush the first one)
        while True:
            self._send(b"\r\n=== UART + USB BOOTLOADER (simulated) ===\r\n[BL] READY\r\n")
            ready, _, _ = select.select([self.master], [], [], 2.0)
            if ready:
                self._buf += os.read(self.master, 4096)
                if b"u" in self._buf:
                    self._buf = self._buf[self._buf.index(b"u") + 1:]
                    break
        self._send(bytes([ACK]))

        size, = struct.unpack("<I", self._read(4))
        if not 0 < size <= SLOT_SIZE:
            self._send(bytes([NACK]))
            self.result = "bad size"
            return
        self._send(bytes([ACK]))                       # "erased"

        while len(self.image) < size:
            self.image += self._read(min(CHUNK_SIZE, size - len(self.image)))
            self._send(bytes([ACK]))                   # "programmed"

        host_crc, = struct.unpack("<I", self._read(4))
        padded = bytes(self.image) + b"\xff" * (SLOT_SIZE - size)
        crc = zlib.crc32(padded) & 0xFFFFFFFF
        if self.corrupt_crc:
            crc ^= 1                                   # flash content differs from the host image
        if crc != host_crc:
            self._send(bytes([NACK]) + b"\r\n[BL] Update : CRC MISMATCH\r\n")
            self.result = "crc mismatch"
            return
        self._send(bytes([ACK]) + b"\r\n[BL] Update : OK\r\n[BL] Jumping to application ...\r\n")
        self.result = "ok"


class BlUploadTest(unittest.TestCase):

    def upload(self, image: bytes, corrupt_crc: bool = False):
        master, slave = pty.openpty()
        tty.setraw(slave)            # no echo / line editing, like a real serial port
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, slave)
        fake = FakeBootloader(master, corrupt_crc)
        fake.start()

        with tempfile.TemporaryDirectory() as tmp:
            app = Path(tmp) / "app.bin"
            app.write_bytes(image)
            proc = subprocess.run(
                [sys.executable, str(TOOL), str(app), "--link", "uart",
                 "--port", os.ttyname(slave), "--wait", "15"],
                capture_output=True, text=True, timeout=60)
        fake.join(timeout=5)
        return proc, fake

    def test_upload_ok(self):
        image = bytes(range(256)) * 20 + b"\x12\x34\x56"    # several chunks + a short last one
        proc, fake = self.upload(image)
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertEqual(fake.result, "ok")
        self.assertEqual(bytes(fake.image), image)
        self.assertIn("=== UPLOADED ===", proc.stdout)
        self.assertIn("[BL] Update : OK", proc.stdout)

    def test_crc_mismatch_fails(self):
        proc, fake = self.upload(b"\xA5" * 1000, corrupt_crc=True)
        self.assertEqual(fake.result, "crc mismatch")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("NACK", proc.stdout)

    def test_rejects_oversized_image(self):
        with tempfile.TemporaryDirectory() as tmp:
            app = Path(tmp) / "big.bin"
            app.write_bytes(b"\x00" * (SLOT_SIZE + 1))
            proc = subprocess.run([sys.executable, str(TOOL), str(app), "--link", "uart",
                                   "--port", "/dev/null"],
                                  capture_output=True, text=True, timeout=30)
        self.assertEqual(proc.returncode, 1)
        self.assertIn("must be 1..", proc.stdout)


if __name__ == "__main__":
    unittest.main()
