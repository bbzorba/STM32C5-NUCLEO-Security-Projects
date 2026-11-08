# Bootloader — UART + USB firmware update (NUCLEO-C562RE)

`Projects/Bootloader` sits at the start of flash. After every reset it waits 5 s — 10 s once a PC
has enumerated its USB port — for a new application over **UART** or **USB**, checks it with a
CRC-32 and starts it. A running application can also be sent back into the bootloader over USB
(1200-baud touch, section 4).

- How it works inside (block diagram, memory map, boot decision, protocol): [ARCHITECTURE.md](ARCHITECTURE.md)
- **Integrity, not authenticity:** the CRC catches broken uploads, not untrusted ones. For signed
  images see `Projects/Secure_Boot`.

All commands are run from the **repository root**.

---

## 1. Connect the board

| Cable | Board connector | Gives you |
|---|---|---|
| **USB0** | ST-LINK USB | SWD (`make flash`, automatic reset) **and** the UART link (ST-LINK virtual COM port) |
| **USB1** | USB-C user port | The USB link: `C562 Bootloader CDC` (`CAFE:B0C5`) while the bootloader runs, `C562 App CDC` (`CAFE:B0C6`) while App_Demo runs |

- Either cable alone is enough for an update on its own link. No wiring is needed.
- USB1 with a USB-C ↔ USB-C cable does **not** power the board. Use USB-A ↔ USB-C, or keep USB0 in.
- Drivers: none to install on Windows, Linux or macOS. Python needs **pyserial** (section 4).

Which cable enables what, and which link `LINK=auto` picks: [ARCHITECTURE.md §5](ARCHITECTURE.md#5-which-cables-give-which-path).

## 2. Quick start

```sh
make submodules                           # once after cloning (TinyUSB, stm32c5xx-dfp, CMSIS_6)
make flash PROJECT_DIR=Projects/Bootloader   # put the bootloader on the board (USB0 / SWD)
make bl-upload                            # build Projects/App_Demo and upload it through the bootloader
```

`make bl-upload` restarts the board by itself: over USB1 with the 1200-baud touch when the
application has a USB port (App_Demo), else over SWD (USB0). With neither, press RESET (section 4).

## 3. LED and button

| LD1 (green) | Meaning |
|---|---|
| on (solid) | Update window after reset: 5 s, 10 s once a PC has enumerated USB1 |
| blinking | Staying in the bootloader — waiting for an update |
| off | Application started (or the board is running something else) |

| You do | Result |
|---|---|
| Hold **B1** while pressing RESET | Bootloader stays until an update arrives |
| Press **B1** during the window | Auto-boot cancelled, bootloader stays |
| 1200-baud touch on the application's USB port | Application restarts into the bootloader, which stays until an update arrives |
| Nothing | After the window the application is checked and started |

With no valid application the bootloader always stays. While it stays, it prints `[BL] READY` every
second on both links, so the upload tool can be started at any time.

## 4. Upload commands

### Pick the link

```sh
make bl-upload LINK=uart          # USB0: ST-LINK virtual COM port
make bl-upload LINK=usb           # USB1: USB-C user port
make bl-upload LINK=auto          # default: USB if the bootloader's or the app's USB port exists, else UART
```

- **Upper case.** make variables are case-sensitive; `link=uart` (and `port=`, `app=`, `usbipd=`) is
  accepted too. Older Makefiles ignored the lower-case spelling, so `LINK` stayed `auto` and picked USB.
- **No fallback between links.** A selected link that is not connected to this PC is an error:

  ```text
  LINK=uart, no ST-LINK:  ERROR: UART link selected, but this PC has no ST-LINK virtual COM port —
                          is USB0 (the ST-LINK cable) connected to this PC? …
  LINK=usb, no USB1:      ERROR: USB link selected, but the bootloader's USB port (CAFE:B0C5) did not
                          appear within 10 s — is USB1 (the USB-C user port) connected to this PC? …
  ```

### Upload over USB1 while the application runs (no USB0, no button)

The application owns USB1 while it runs, so only the application itself can hand over to the
bootloader. App_Demo enumerates its own port, `C562 App CDC` (`CAFE:B0C6`). Opening that port at
**1200 baud and closing it** (the "1200-baud touch", as on Arduino and Raspberry Pi Pico boards)
makes it reset into the bootloader, which then **stays** until the upload arrives. The upload tool
does this by itself:

```sh
make bl-enter                                                         # only enter the bootloader, it stays
make bl-upload LINK=usb                                               # build + touch + upload
python3 tools/bl_upload.py build/Projects/App_Demo/App_Demo.bin --link usb   # ready binary, any PC
```

```text
[0/4] Application is running ...
  1200-baud touch on COM18 — the application restarts into the bootloader
=== Bootloader upload: App_Demo.bin ===
  Port        : COM17
```

The request is one TAMP backup register (`bl_request.h`): it survives the reset, the bootloader
clears it, and a power cycle clears it too. A normal reset afterwards boots the application again.

**Adding it to your own application** (Root_of_Trust and Secure_FW_Update do not have it yet — use
SWD or B1 + RESET for them):

```make
DEPS := ... USB_CDC Bootloader          # module.mk; startup.c: [16 + 62] = USB_DRD_FS_IRQHandler
```

```c
USB_CDC_SetIdentity(USB_CDC_APP_PID, "My App CDC");   /* CAFE:B0C6 — what the tool looks for */
USB_CDC_Init();
while (1) {
    USB_CDC_Task();
    if (USB_CDC_TakeTouchEvent())
        BL_RequestBootloader();                         /* see Projects/App_Demo/src/main.c */
    /* ... */
}
```

### Pick the image

```sh
make bl-upload                                        # Projects/App_Demo (built first)
make bl-upload PROJECT_DIR=Projects/Root_of_Trust     # build + upload another app
make bl-upload APP=build/Projects/App_Demo/App_Demo.bin   # upload a ready binary, no build
make bl-upload APP=/path/to/my_app.bin LINK=usb       # any .bin linked at 0x08010000
```

`make bl-upload` uploads the selected `PROJECT_DIR` if it is linked at `0x08010000`, otherwise
`Projects/App_Demo`.

### Pick the port

Order: `PORT` if set → auto-detected port (bootloader USB port or ST-LINK VCP) → `DEFAULT_PORT`
(UART only, and only if that port exists on this PC).

```sh
make com-list                                 # list serial ports (Windows / WSL)
make bl-upload LINK=uart PORT=COM4            # Windows
make bl-upload LINK=uart PORT=/dev/ttyACM0    # Linux
make bl-upload DEFAULT_PORT=COM7              # fallback when no ST-LINK is detected
```

Set your board's ST-LINK port once in the top-level `Makefile` (`DEFAULT_PORT ?= COMx`). Windows
shows it as *STMicroelectronics STLink Virtual COM Port*.

### Without the SWD reset

Use this when USB0 is not connected to this PC, or to control the timing yourself:

1. Hold **B1** and press **RESET** (LD1 blinks), or just press RESET within the next 30 s.
2. Run the tool without `--reset`:

```sh
python3 tools/bl_upload.py build/Projects/App_Demo/App_Demo.bin --link usb
python3 tools/bl_upload.py build/Projects/App_Demo/App_Demo.bin --link uart --port COM4
```

### `tools/bl_upload.py` options

```text
python3 tools/bl_upload.py <app.bin> [--link auto|uart|usb] [--port PORT]
                           [--fallback-port PORT] [--reset] [--wait SECONDS] [--wsl-usbipd]
```

| Option | Meaning |
|---|---|
| `--link` | `uart`, `usb` or `auto` (default) |
| `--enter` | Only put the board into the bootloader over USB1 (1200-baud touch), no binary — `make bl-enter` |
| `--port` | Force a serial port |
| `--fallback-port` | UART port used when no ST-LINK is detected (`make` passes `DEFAULT_PORT`) |
| `--reset` | Reset the board over SWD first (STM32_Programmer_CLI, else `st-flash`, else asks you to press RESET) |
| `--wait` | Seconds to wait for `[BL] READY` / the USB port (default 30) |
| `--wsl-usbipd` | WSL2: the bootloader's USB port is attached with usbipd — use `/dev/ttyACM*` |

| OS | Install pyserial |
|---|---|
| Debian / Ubuntu / WSL | `sudo apt install python3-serial` |
| Windows | `py -m pip install pyserial` |
| Other Linux | `python3 -m venv .venv && .venv/bin/pip install pyserial` |

## 5. Check which link was used

The tool prints the link and port it chose, and then the board's own log:

```text
  Link        : UART (ST-LINK VCP)
  Port        : COM13 @ 115200
  ...
    [BL] Update : OK                      <- printed by the bootloader after receiving the image
      CRC tag @ 0x0803C000 : BE4981F0     <- tag written by the bootloader
      Computed CRC         : BE4981F0
```

- **SWD does not write flash during `bl-upload`.** The only SWD command is the reset
  (`STM32_Programmer_CLI -c port=SWD -rst`, printed by the tool). The image itself always travels
  over the selected serial link.
- **To rule SWD out completely**, upload without `--reset` (section 4) and press RESET yourself.
- `[BL] Update : OK` and a matching CRC tag can only come from the bootloader. `make flash` over SWD
  clears the tag instead, and the bootloader would then refuse to boot the image.

### What is on the board?

`make read-flash` (USB0) names the image at 0x08000000 and the one in the active slot, shows the
slot's CRC tag status and which local build each one matches — without stopping the MCU.

## 6. Windows, Linux and WSL2

| Host | What to do |
|---|---|
| **Windows** | Works directly with `COMx` ports |
| **Native Linux** | Uses `/dev/ttyACM*`. Once: `sudo usermod -aG dialout $USER`, then log in again |
| **WSL2, UART** | Nothing extra. The tool re-runs itself with the Windows Python (`python.exe`, needs pyserial) to open the ST-LINK `COMx` port |
| **WSL2, USB** | Attach the bootloader's USB port with usbipd — see below |

### WSL2 + usbipd (USB link)

Keep **USB0 (ST-LINK) on Windows**: `make flash`, the SWD reset and the UART link use it from there.
Attach only USB1 — the bootloader's (`CAFE:B0C5`) and App_Demo's (`CAFE:B0C6`) port — to WSL.
While an `usbipd attach` runs, **Windows itself cannot use these ports** (they vanish from Windows
~1 s after each reset). Stop it when you upload from Windows instead.

**1. Windows — install usbipd (PowerShell as administrator, once)**

```powershell
winget install --exact dorssel.usbipd-win
# close and reopen PowerShell so `usbipd` is on PATH
```

**2. Windows — share the bootloader's USB port (PowerShell as administrator, once)**

```powershell
usbipd policy add --effect Allow --operation AutoBind --hardware-id CAFE:B0C5
usbipd policy add --effect Allow --operation AutoBind --hardware-id CAFE:B0C6
```

This works even while the devices are not plugged in. (`usbipd bind --busid …` also works, but only
while the device exists.)

**3. Find the BUSID.** Hold B1 + RESET so the bootloader stays, then:

```powershell
usbipd list     # "CAFE:B0C5  USB Serial Device (COMx)" — note its BUSID, e.g. 2-3
```

If the device is already gone, Windows still remembers where it was:
`Get-PnpDevice | ? InstanceId -match 'VID_CAFE&PID_B0C5\\'` gives the instance ID, and its
`DEVPKEY_Device_LocationInfo` (`Get-PnpDeviceProperty`) reads `Port_#0003.Hub_#0002` → BUSID `2-3`.
The BUSID belongs to the physical USB port, so keep USB1 in the same port.

**4. Windows — attach to WSL (normal PowerShell, every session, leave it open)**

```powershell
usbipd attach --wsl --busid 2-3 --auto-attach --unplugged
```

The device disappears on every jump to the application and reappears on every reset;
`--auto-attach` re-attaches it each time.

**5. WSL — once**

```sh
sudo usermod -aG dialout $USER      # serial access, then reopen the WSL terminal
sudo apt install python3-serial     # pyserial
```

**6. WSL — upload**

```sh
make bl-upload LINK=usb USBIPD=1                 # build + upload Projects/App_Demo over USB
make bl-upload LINK=usb USBIPD=1 APP=x.bin       # a ready binary
python3 tools/bl_upload.py build/Projects/App_Demo/App_Demo.bin --link usb --wsl-usbipd   # without USB0: press RESET
```

| Check | Expected while the bootloader runs |
|---|---|
| `usbipd list` (Windows) | `CAFE:B0C5` shows **Attached** |
| `lsusb` (WSL, package `usbutils`) | `cafe:b0c5` listed |
| `dmesg \| tail` (WSL) | `cdc_acm … ttyACM0: USB ACM device` |

With App_Demo the touch makes the bootloader stay, so the re-attach has all the time it needs.
For other applications hold B1 while resetting so the bootloader keeps waiting.

## 7. Other ways to load an application

| Method | Command | Notes |
|---|---|---|
| SWD + seal | `make seal-active PROJECT_DIR=Projects/App_Demo` | Flashes the slot and writes the CRC tag with `tools/seal.py` |
| SWD only | `make flash PROJECT_DIR=Projects/App_Demo` | Clears the tag — the bootloader will **not** boot it until it is sealed or uploaded |
| ST ROM bootloader (USB DFU) | not integrated yet | Needs the `BOOT_SEL` / `BOOT0` option bytes; writes flash but not the CRC tag |

## 8. Settings (`inc/bootloader.h`)

| Macro | Default | Meaning |
|---|---|---|
| `BL_WAIT_MS` | `5000` | Update window after reset |
| `BL_USB_MOUNT_GRACE_MS` | `10000` | Window left after a PC enumerates the USB port |
| `BL_USB_OPEN_GRACE_MS` | `3000` | Time left after the PC opens the USB port |
| `BL_READY_REPEAT_MS` | `1000` | `[BL] READY` period while staying |
| `BL_BTN_PORT` / `BL_BTN_PIN` / `BL_BTN_ACTIVE` | PC13, high | B1 (active **high** on the NUCLEO-C562RE) |
| `BL_LED_PORT` / `BL_LED_PIN` | PA5 | LD1 |
| `BL_USB_VBUS_SENSE` | `0` | `1` + `BL_USB_VBUS_PORT` / `_PIN`: USB pull-up only while VBUS is present. The board's VBUS-sense pin is not confirmed yet (MB2213 schematic) |

USB identity (VID/PID, strings): `Drivers/USB_CDC/inc/usb_cdc.h`.

## 9. Troubleshooting

| Symptom | Check |
|---|---|
| `bootloader READY not seen` | Is `Projects/Bootloader` flashed? Hold B1 + RESET and retry. Right port (`make com-list`)? |
| USB port `CAFE:B0C5` never appears | Cable in USB1 (user port, not ST-LINK)? Board powered? WSL: is `usbipd attach --auto-attach` running? |
| `LINK=auto` uses UART although USB1 is connected | The running application has no USB port (only App_Demo has one) — use `LINK=usb` and press RESET |
| `make bl-upload link=uart` uploads over USB | Makefile older than the lower-case support: write `LINK=uart` |
| `UART link selected, but this PC has no ST-LINK …` | USB0 is not connected to this PC — connect it, or use `LINK=usb` |
| `USB link selected, but … did not appear within 10 s` | USB1 is not connected to this PC (or WSL without usbipd attach) — connect it, or use `LINK=uart` |
| Windows shows the bootloader's COM port only ~1–2 s | A `usbipd attach --auto-attach` (WSL) takes the device — stop it, or upload from WSL with `USBIPD=1` |
| Windows plays connect / disconnect sounds | Normal: the USB device exists only while the bootloader runs |
| "Unknown USB Device (Device Descriptor Request Failed)" | Old bootloader firmware — reflash `Projects/Bootloader` |
| UART upload: `NACK`, board says `Update : TIMEOUT` | Old bootloader firmware without the USART2 RX FIFO fix — reflash `Projects/Bootloader` |
| `cannot open /dev/ttyACM0: Permission denied` | `sudo usermod -aG dialout $USER`, then log in again |
| `pyserial is not installed` | See the install table in section 4 |
| WSL: `No STM32_Programmer_CLI` / `No debug probe detected` | USB0 not connected to this PC — press RESET yourself |
| Upload OK but the app does not start | Is the app linked at `0x08010000`? Watch the UART log: tag / CRC / MSP lines |
| `USB_CDC: submodules missing` when building | `make submodules` |
