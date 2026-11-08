# STM32C5 NUCLEO-C562RE — Bare-Metal Security Projects

Register-level drivers and security projects for the **NUCLEO-C562RE** (STM32C562RE, Cortex-M33).
No HAL, no CubeMX. CMSIS core is used only for the NVIC; every peripheral is accessed through
hand-written register structs. Build: GNU Make + arm-none-eabi-gcc. Debug: VS Code + Cortex-Debug + OpenOCD.

## Repository

| Folder | Content |
|---|---|
| `Drivers/` | One folder per peripheral (GPIO, UART, AES, CRC, DMA, FDCAN, HASH, NVIC, RNG, SysTick, USB_CDC). Each is a library **and** a standalone demo |
| `Projects/` | Bootloader, Secure_Boot, App_Demo, Root_of_Trust, Secure_FW_Update, ECDSA, RSA, FLASH |
| `linker/` | Shared linker scripts: one memory map, three layouts |
| `tools/` | Upload, seal, flash-read and CI scripts |
| `build/` | Build output, generated and git-ignored |

Submodules: `Drivers/TinyUSB`, `Drivers/stm32c5xx-dfp`, `Drivers/CMSIS_6` (needed by USB_CDC).

## Setup

- `git clone --recurse-submodules <repo>`, or `make submodules` in an existing clone
- **arm-none-eabi-gcc** (GCC 12+) and **GNU Make** on PATH. Without them on PATH, the Makefile uses
  the ones in STM32CubeIDE (`TOOLCHAIN_DIR=` / `CUBE_PROG=` to override)
- **STM32CubeProgrammer** (`STM32_Programmer_CLI`) to flash, **Python 3.8+** with **pyserial** for the tools
- For debugging: **arm-none-eabi-gdb**, **OpenOCD 0.12+**, the VS Code extension **Cortex-Debug**

Works on Windows, Linux and WSL2. From WSL the Windows STM32CubeProgrammer and OpenOCD are used, so
the ST-LINK stays on Windows.

## Build and flash

```sh
make                                        # help: targets, variables, buildable modules
make build PROJECT_DIR=Drivers/GPIO         # build one module
make flash PROJECT_DIR=Drivers/GPIO         # build + flash over SWD (ST-LINK, USB0)
make monitor                                # serial console (ST-LINK virtual COM port)
make read-flash                             # show which images are in flash
make clean / make clean-all
```

Output: `build/<PROJECT_DIR>/<Module>.elf/.bin/.hex/.map`. `DEBUG=1` builds go to `build/debug/`.
The default `PROJECT_DIR` is set at the top of the `Makefile`.

Each module's `module.mk` defines its sources (`<Name>_SRC_C`, `<Name>_INC`) for the modules that use
it. Inside `ifeq ($(MODULE),<Name>)` it also defines how to build it alone: `DEPS`, `LAYOUT`,
`STACK_SIZE` and any extra targets. To add a module, add a folder with a `module.mk`.

### Memory layouts

| `LAYOUT` | Flash | Used by |
|---|---|---|
| `standalone` (default) | `0x08000000`, 512 KB | Driver demos, ECDSA, RSA, FLASH |
| `bootloader` | `0x08000000`, 64 KB (sectors 0–7) | Bootloader, Secure_Boot |
| `app_slot` | `0x08010000`, 176 KB (sectors 8–29) | App_Demo, Root_of_Trust, Secure_FW_Update |

The addresses are defined once, in `linker/memory_map.ld`. Every image carries an `IMGID:<module>`
record after its vector table, which `make read-flash` reads to name the images in flash.

## Bootloader

`Projects/Bootloader` boots the application in the active slot, and updates it over UART
(ST-LINK virtual COM port) or USB (USB-C user port):

```sh
make flash PROJECT_DIR=Projects/Bootloader  # once, over SWD
make bl-upload                              # build Projects/App_Demo and upload it (link=auto)
make bl-upload LINK=usb                     # or LINK=uart
make bl-enter                               # only put the board into the bootloader over USB
```

User guide: [Projects/Bootloader/README.md](Projects/Bootloader/README.md).
How it works: [Projects/Bootloader/ARCHITECTURE.md](Projects/Bootloader/ARCHITECTURE.md).

## Debugging (VS Code)

Press **F5** and pick a configuration. The pre-launch task builds with `DEBUG=1` and flashes only
what has changed.

| Configuration | Debugs |
|---|---|
| **Debug: Bootloader + application** | Bootloader and App_Demo in one session. The session starts at the bootloader's `main` and stops again at the app's `main`. Breakpoints work in both |
| **Debug: Active App (from Makefile)** | A standalone `PROJECT_DIR` (driver demos, ECDSA, RSA, …) |

GDB starts OpenOCD itself through `tools/openocd-pipe.sh`, so no server or TCP port is needed.
`$OPENOCD` selects OpenOCD, otherwise `openocd`/`openocd.exe` on PATH. The log is `build/openocd.log`.
Without VS Code, use `make debug-boot` or `make debug`, then:

```sh
arm-none-eabi-gdb build/debug/bootloader.elf -ex "target extended-remote | tools/openocd-pipe.sh"
```

## CI

GitHub Actions ([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) runs on GitHub-hosted runners
for every push and pull request:

- **Lint:** cppcheck and ruff
- **Build:** every module; any warning fails the build
- **Host tools:** tests on Python 3.8 and 3.12
- **Verify:** load address, sizes, vector table

Each run attaches every module's binaries plus a firmware bundle with a `manifest.json`. Flashing and
uploads stay manual tests on the board.

## Troubleshooting

| Problem | Fix |
|---|---|
| Toolchain / programmer not found | Put it on PATH or set `TOOLCHAIN_DIR=` / `CUBE_PROG=` |
| `USB_CDC: submodules missing` | `make submodules` |
| Breakpoints never hit | Wrong `PROJECT_DIR`, or not a `DEBUG=1` build — use the F5 configurations |
| `make bl-upload` problems | [Bootloader troubleshooting](Projects/Bootloader/README.md#7-troubleshooting) |
