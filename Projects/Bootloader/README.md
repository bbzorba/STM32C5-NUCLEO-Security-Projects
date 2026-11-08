# Bootloader — UART + USB firmware update

`Projects/Bootloader` sits at `0x08000000`. After every reset it waits 5 s for a new application,
over **UART** or **USB**, then checks the application with a CRC-32 and starts it. Once a PC has
enumerated its USB port, the wait becomes 10 s.

The CRC catches broken uploads, not untrusted ones. For signed images see `Projects/Secure_Boot`.
To learn how the bootloader works, read [ARCHITECTURE.md](ARCHITECTURE.md).

Run all commands from the repository root.

## 1. Connect the board

| Cable | Connector | Gives you |
|---|---|---|
| **USB0** | ST-LINK | SWD (flash, reset) and the **UART** link (ST-LINK virtual COM port) |
| **USB1** | USB-C user port | The **USB** link. The bootloader appears as `C562 Bootloader CDC` (`CAFE:B0C5`), App_Demo as `C562 App CDC` (`CAFE:B0C6`) |

Either cable alone is enough to update over its own link. A USB-C ↔ USB-C cable on USB1 does not
power the board: use USB-A ↔ USB-C, or keep USB0 connected.

## 2. Quick start

```sh
make flash PROJECT_DIR=Projects/Bootloader   # once, over SWD (USB0)
make bl-upload                               # build Projects/App_Demo and upload it
```

`make bl-upload` restarts the board into the bootloader by itself. Over USB1 it uses a 1200-baud
touch on App_Demo's port; otherwise it resets over SWD. If neither is possible, press RESET.

## 3. Commands

```sh
make bl-upload LINK=usb                       # upload over USB1
make bl-upload LINK=uart PORT=COM4            # upload over USB0 (ST-LINK virtual COM port)
make bl-upload PROJECT_DIR=Projects/Root_of_Trust   # another application
make bl-upload APP=path/to/app.bin            # a ready binary, linked at 0x08010000
make bl-enter                                 # only enter the bootloader over USB1; it stays
make read-flash                               # which bootloader + application are in flash (USB0)
make com-list                                 # list serial ports
```

- `LINK=auto` (default) picks USB when the bootloader's or the app's USB port is present, otherwise UART.
- If the selected link is not connected to this PC, the command fails. There is no fallback to the
  other link.
- The variables can also be written in lower case (`link=usb`, `port=`, `app=`, `usbipd=`).
- `make bl-upload` uploads the selected `PROJECT_DIR` if it is linked at `0x08010000`, otherwise
  App_Demo.
- To set your ST-LINK port once, add `DEFAULT_PORT ?= COMx` to the top-level `Makefile`.

Without make: `python3 tools/bl_upload.py <app.bin>`. Options: `--link auto|uart|usb`, `--port`,
`--reset` (SWD reset first), `--enter`, `--wait`, `--wsl-usbipd`.

## 4. LED and button

| LD1 | Meaning |
|---|---|
| solid | Update window after reset |
| blinking | Staying in the bootloader, waiting for an update |
| off | Application running |

The bootloader **stays** (and prints `[BL] READY` every second) in any of these cases:

- B1 is held during reset
- B1 is pressed during the window
- the application requested it with a 1200-baud touch
- there is no valid application

## 5. Entering the bootloader from your own application

While an application runs, it owns USB1. To be updatable over USB1 alone, the application must react
to the 1200-baud touch, as App_Demo does:

```make
DEPS := ... USB_CDC Bootloader          # module.mk; startup.c: [16 + 62] = USB_DRD_FS_IRQHandler
```

```c
USB_CDC_SetIdentity(USB_CDC_APP_PID, "My App CDC");   /* CAFE:B0C6, which the upload tool looks for */
USB_CDC_Init();
while (1) {
    USB_CDC_Task();
    if (USB_CDC_TakeTouchEvent())
        BL_RequestBootloader();                         /* resets into the bootloader */
}
```

Applications without the touch (Root_of_Trust, Secure_FW_Update) need the SWD reset or B1 + RESET.

## 6. WSL2: USB link through usbipd

The UART link needs nothing extra: from WSL the tool uses the Windows Python and the `COMx` port.
For the USB link, keep USB0 on Windows and attach only USB1 to WSL:

```powershell
# Windows, PowerShell as administrator, once
winget install --exact dorssel.usbipd-win
usbipd policy add --effect Allow --operation AutoBind --hardware-id CAFE:B0C5
usbipd policy add --effect Allow --operation AutoBind --hardware-id CAFE:B0C6

# Windows, every session; leave it running (BUSID from `usbipd list`, while the bootloader runs)
usbipd attach --wsl --busid <BUSID> --auto-attach --unplugged
```

```sh
# WSL, once: sudo usermod -aG dialout $USER && sudo apt install python3-serial
make bl-upload LINK=usb USBIPD=1
```

While `usbipd attach` runs, Windows itself cannot use these ports. Stop it before you upload from Windows.

## 7. Troubleshooting

| Symptom | Fix |
|---|---|
| `bootloader READY not seen` | Is the bootloader flashed? Hold B1 + RESET and retry. Is it the right port (`make com-list`)? |
| `UART link selected, but this PC has no ST-LINK …` | Connect USB0, or use `LINK=usb` |
| `USB link selected, but … did not appear within 10 s` | Connect USB1. On WSL, check that `usbipd attach` is running |
| `LINK=auto` uses UART although USB1 is connected | The running application has no USB port: use `LINK=usb` and press RESET |
| The Windows COM port vanishes after ~1 s | A WSL `usbipd attach` holds it. Stop it, or upload from WSL |
| `Permission denied` on `/dev/ttyACM0` | `sudo usermod -aG dialout $USER`, then log in again |
| Upload OK, but the application does not start | Is it linked at `0x08010000`? Check the UART log |
| After `make flash` of an app, the bootloader does not boot it | `make flash` clears the CRC tag. Use `make seal-active` or `make bl-upload` |

## Settings (`inc/bootloader.h`)

| Macro | Default | Meaning |
|---|---|---|
| `BL_WAIT_MS` | `5000` | Update window after reset |
| `BL_USB_MOUNT_GRACE_MS` | `10000` | Minimum window once a PC enumerates USB1 |
| `BL_USB_OPEN_GRACE_MS` | `3000` | Minimum time left once the PC opens the port |
| `BL_BTN_*` / `BL_LED_*` | PC13 / PA5 | B1 and LD1 |

The USB VID/PID and strings are set in `Drivers/USB_CDC/inc/usb_cdc.h`.
