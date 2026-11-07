# Bootloader — UART + USB firmware update (NUCLEO-C562RE)

`Projects/Bootloader` is a small bare-metal bootloader. It lives at the start of flash, receives
a new application over **UART** or **USB**, checks it with a CRC-32 and starts it.

> **Integrity, not authenticity.** The CRC-32 catches a corrupted or incomplete image. It does not
> prove who built the image — anyone can compute a CRC. For signed images see `Projects/Secure_Boot`.

All commands below are run from the **repository root**.

---

## 1. Memory map

| Address | Sectors | Content |
|---|---|---|
| `0x08000000` | 0–7 (64 KB) | Bootloader — the chip always starts here after reset |
| `0x08010000` | 8–29 (176 KB) | Active slot — the application |
| `0x0803C000` | 30 | CRC-32 tag of the active slot (`0xFFFFFFFF` = not sealed) |

Applications for this bootloader are linked at `0x08010000` (`Projects/App_Demo`,
`Projects/Root_of_Trust`, `Projects/Secure_FW_Update`). The map is the same as `Projects/Secure_Boot`
and `tools/seal.py`.

## 2. What happens after a reset

| Situation | LD1 (green) | Behaviour |
|---|---|---|
| Normal reset | on (solid) | 5 s update window: waits for `u` on UART **and** USB |
| `u` received on UART or USB | on | Receives the image on that link, verifies it, writes the tag, boots it |
| USB COM port opened by the PC | on | Sends `[BL] READY` on USB and extends the window by 3 s |
| B1 pressed during the window | blinking | Auto-boot cancelled — stays in the bootloader |
| B1 held while pressing RESET | blinking | Stays in the bootloader until an update arrives |
| No valid application | blinking | Stays in the bootloader until an update arrives |
| Window ends, nothing received | off | Checks tag + CRC + vector table, then jumps to the application |

While it stays in the bootloader it repeats `[BL] READY` every second on both links, so the upload
tool can be started at any time.

Before jumping, the bootloader disconnects USB and resets the USB peripheral. The PC sees an unplug,
and the application starts with the peripherals in their reset state.

## 3. Update links

| Link | Cable / connector | MCU pins | PC side |
|---|---|---|---|
| **UART** | ST-LINK USB cable | USART2: PA2 TX, PA3 RX (115200 8N1) | ST-LINK virtual COM port (`COMx` / `/dev/ttyACM*`) |
| **USB** | USB-C **user** connector | USB_DRD_FS: PA11 DM, PA12 DP | `C562 Bootloader CDC`, VID:PID `CAFE:B0C5` |

- **USB driver:** the USB link is a CDC ACM virtual COM port (TinyUSB, `Drivers/USB_CDC`).
  Windows, Linux and macOS have built-in drivers, so nothing needs installing.
- **USB port lifetime:** the USB COM port exists only while the bootloader is running. Every reset
  therefore gives a Windows "device connected" sound, and about 5 s later "device disconnected"
  when the bootloader jumps to the application. That is expected — not a power or cable problem.
- **READY on USB:** about 150 ms after the PC opens the COM port, then every second. It is not sent
  immediately, because Windows serial libraries (pyserial, PuTTY …) purge their receive buffer right
  after they assert DTR, so an immediate READY would be lost.
- **UART FIFO:** the bootloader enables the 8-byte USART2 RX FIFO. Servicing USB can delay a UART
  poll by up to ~170 µs (measured), longer than one byte at 115200 baud (87 µs). Without the FIFO,
  UART uploads lost bytes and timed out while a PC was attached to the USB-C port.
- **No wiring:** neither link needs wires — both use the board's own USB connectors.
- **Powering from the USB-C port:** a USB-C ↔ USB-C cable does **not** power the board on its own.
  Use a USB-A ↔ USB-C cable, or keep the ST-LINK cable plugged in too.
- **VID:PID:** `CAFE:B0C5` is TinyUSB's test vendor ID, fine for development only.

## 4. Quick start

```sh
# 0. Once after cloning: fetch TinyUSB / stm32c5xx-dfp / CMSIS_6 (git submodules)
make submodules

# 1. Put the bootloader on the board (over SWD, ST-LINK cable)
make flash PROJECT_DIR=Projects/Bootloader

# 2. Upload an application through the bootloader
make bl-upload                    # builds + uploads Projects/App_Demo, link chosen automatically
```

`make bl-upload` works with any `PROJECT_DIR`:

- **Default image:** it builds and uploads the selected project if it runs in the active slot
  (`FLASH_ADDR = 0x08010000`), otherwise `Projects/App_Demo`.
- **Reset:** it resets the board over SWD first (`--reset`), so the 5 s window opens by itself.

## 5. Upload commands

### Pick the link

```sh
make bl-upload LINK=auto          # default: USB if the bootloader's USB port is present, else UART
make bl-upload LINK=usb           # USB-C user connector
make bl-upload LINK=uart          # ST-LINK virtual COM port
```

### Pick the image

```sh
make bl-upload                                        # Projects/App_Demo (built first)
make bl-upload PROJECT_DIR=Projects/Root_of_Trust     # build + upload another active-slot app
make bl-upload APP=Projects/App_Demo/main.bin         # upload a ready binary, no build
make bl-upload APP=/path/to/my_app.bin LINK=usb       # any .bin linked at 0x08010000
```

### Pick the port

The port is chosen in this order:
1. `PORT`, if you set it;
2. the auto-detected port — the bootloader's USB port (`CAFE:B0C5`) or the ST-LINK virtual COM port;
3. `DEFAULT_PORT` (UART only).

```sh
make bl-upload LINK=uart PORT=COM4            # Windows
make bl-upload LINK=uart PORT=/dev/ttyACM0    # Linux
make bl-upload DEFAULT_PORT=COM7              # fallback when no ST-LINK is detected
make com-list                                 # list the serial ports (Windows / WSL)
```

Set your board's port once in the top-level `Makefile` (`DEFAULT_PORT ?= COM12` — the ST-LINK port of the board used here). Windows shows it in
Device Manager as *STMicroelectronics STLink Virtual COM Port*.

### Without the SWD reset (bootloader already waiting)

If the ST-LINK is not available, or you want to control the timing yourself:

1. Hold **B1** and press **RESET**. LD1 blinks and the bootloader stays.
2. Run the upload tool directly, without `--reset`:

```sh
python3 tools/bl_upload.py Projects/App_Demo/main.bin --link usb
python3 tools/bl_upload.py Projects/App_Demo/main.bin --link uart --port COM4
```

The tool waits up to 30 s for `[BL] READY` (`--wait N` to change it).

### `tools/bl_upload.py` options

```text
python3 tools/bl_upload.py <app.bin> [--link auto|uart|usb] [--port PORT]
                           [--fallback-port PORT] [--reset] [--wait SECONDS] [--wsl-usbipd]
```

| Option | Meaning |
|---|---|
| `--link` | `uart`, `usb` or `auto` (default) |
| `--port` | Force a serial port |
| `--fallback-port` | UART port used when no ST-LINK is detected (`make` passes `DEFAULT_PORT`) |
| `--reset` | Reset the board over SWD first (STM32_Programmer_CLI, else `st-flash`, else asks you to press RESET) |
| `--wait` | Seconds to wait for `[BL] READY` / the USB port (default 30) |
| `--wsl-usbipd` | WSL2: the board's USB devices are attached with usbipd — use `/dev/ttyACM*` |

Python needs **pyserial**:

| OS | Install |
|---|---|
| Debian / Ubuntu / WSL | `sudo apt install python3-serial` |
| Windows | `py -m pip install pyserial` |
| Other Linux | `python3 -m venv .venv && .venv/bin/pip install pyserial` |

## 6. Windows, Linux and WSL2

| Host | What to do |
|---|---|
| **Windows** | Works directly with `COMx` ports |
| **Native Linux** | Uses `/dev/ttyACM*`. Add yourself to the serial group once: `sudo usermod -aG dialout $USER`, then log in again |
| **WSL2, no usbipd** | `make bl-upload` re-runs the tool with the Windows Python (`python.exe`), which opens the `COMx` ports |
| **WSL2 + usbipd** | See below |

### WSL2 + usbipd

Keep the **ST-LINK on Windows**: `make flash` and the SWD reset use the Windows
STM32CubeProgrammer. Attach only the **USB-C user port** (the bootloader's `CAFE:B0C5` device)
to WSL. Installing usbipd and `usbipd bind` need administrator rights, once.

**1. Windows — install (PowerShell as administrator, once)**

```powershell
winget install --exact dorssel.usbipd-win
# close and reopen PowerShell so `usbipd` is on PATH
```

**2. Make the bootloader's USB device appear.** Run one of these:
- Hold B1 and press RESET.
- Erase the CRC tag over SWD, then reset, so the bootloader stays:
  `STM32_Programmer_CLI -c port=SWD -e 30 -rst`

**3. Windows — share it (PowerShell as administrator, once per device)**

```powershell
usbipd list                     # find the line "CAFE:B0C5  USB Serial Device (COMx)" and note its BUSID, e.g. 2-3
usbipd bind --busid 2-3         # shared from now on (survives reboots)
```

**4. Windows — attach to WSL (normal PowerShell, every session)**

```powershell
usbipd attach --wsl --busid 2-3 --auto-attach
# leave this window open: the device disappears on every jump to the application
# and reappears on every reset — --auto-attach re-attaches it to WSL each time
```

**5. WSL — once**

```sh
sudo usermod -aG dialout $USER      # serial access, then close and reopen the WSL terminal
sudo apt install python3-serial     # pyserial (if not installed yet)
```

**6. WSL — upload**

```sh
ls /dev/ttyACM*                                  # present while the bootloader runs
make bl-upload LINK=usb USBIPD=1                 # build + upload Projects/App_Demo over USB
make bl-upload LINK=usb USBIPD=1 APP=x.bin       # a ready binary
```

| Check | Expected |
|---|---|
| `usbipd list` (Windows) | `CAFE:B0C5` shows **Attached** while the bootloader runs |
| `lsusb` (WSL, package `usbutils`) | `cafe:b0c5` listed |
| `dmesg \| tail` (WSL) | `cdc_acm … ttyACM0: USB ACM device` |

- **Timing:** if re-attaching after the SWD reset takes longer than the bootloader window
  (5 s + 3 s once the port is opened), hold B1 while resetting so the bootloader keeps waiting.
- **UART from WSL:** `LINK=uart` keeps working without usbipd, because the ST-LINK stays on Windows
  and `bl_upload.py` uses the Windows Python for it. Drop `USBIPD=1` for UART uploads.

## 7. Other ways to load an application

| Method | Command | Notes |
|---|---|---|
| SWD + seal (no bootloader protocol) | `make seal-active PROJECT_DIR=Projects/App_Demo` | Flashes the slot and writes the CRC tag with `tools/seal.py` |
| SWD only | `make flash PROJECT_DIR=Projects/App_Demo` | Clears the old tag; the bootloader will **not** boot it until it is sealed or uploaded |
| ST ROM bootloader (USB DFU) | not integrated yet | Built into the STM32C5. It needs the `BOOT_SEL` / `BOOT0` option bytes. It writes flash but not our CRC tag. Planned together with the BOOT0 pin |

## 8. Protocol (same bytes on UART and USB)

```text
host -> 'u'                          board -> ACK (0x79)
host -> image size, u32 little-endian board -> erases sectors 8-30, ACK
host -> data chunk (<= 256 bytes)     board -> programs + reads back, ACK   (repeat)
host -> CRC-32 of the image padded    board -> compares with the hardware CRC,
        with 0xFF to 176 KB                    writes the tag, ACK
```

- **Errors:** any failure is answered with NACK (`0x1F`), and the slot is left without a tag, so a
  broken upload is never booted.
- **CRC:** `zlib.crc32`, the same value `tools/seal.py` writes.

## 9. Configuration (`inc/bootloader.h`)

| Macro | Default | Meaning |
|---|---|---|
| `BL_WAIT_MS` | `5000` | Update window after reset |
| `BL_USB_OPEN_GRACE_MS` | `3000` | Extra time after the USB COM port is opened |
| `BL_READY_REPEAT_MS` | `1000` | `[BL] READY` period while staying in the bootloader |
| `BL_BTN_PORT` / `BL_BTN_PIN` / `BL_BTN_ACTIVE` | PC13, high | B1 — active **high** on the NUCLEO-C562RE (external pull-down) |
| `BL_LED_PORT` / `BL_LED_PIN` | PA5 | LD1 |
| `BL_USB_VBUS_SENSE` | `0` | `1` + `BL_USB_VBUS_PORT` / `BL_USB_VBUS_PIN`: D+ pull-up only while VBUS is present. The NUCLEO-C562RE VBUS-sense pin is not confirmed yet — check the MB2213 schematic |

USB identity (VID/PID, strings) is in `Drivers/USB_CDC/inc/usb_cdc.h`.

## 10. Troubleshooting

| Symptom | Check |
|---|---|
| `bootloader READY not seen` | Is `Projects/Bootloader` flashed? Hold B1 + RESET and retry. Is the port right (`make com-list`)? |
| USB port `CAFE:B0C5` never appears | Cable in the USB-C **user** connector (not ST-LINK)? Board powered? With WSL: is `usbipd attach --auto-attach` running? |
| Windows plays connect / disconnect sounds | Normal: the USB device exists only while the bootloader runs (see section 3) |
| "Unknown USB Device (Device Descriptor Request Failed)" | Bootloader firmware older than the UID fix — reflash `Projects/Bootloader` |
| UART upload: `NACK`, board says `Update : TIMEOUT` | Bootloader firmware without the USART2 RX FIFO fix — reflash `Projects/Bootloader` |
| `cannot open /dev/ttyACM0: Permission denied` | `sudo usermod -aG dialout $USER`, then log in again |
| `pyserial is not installed` | See the install table in section 5 |
| Upload OK but the app does not start | Is the app linked at `0x08010000`? Watch the UART log: tag / CRC / MSP lines |
| `USB_CDC: submodules missing` when building | `make submodules` |
