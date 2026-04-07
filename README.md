# STM32C5 NUCLEO-C562RE Bare-Metal Projects (VS Code)

This repository provides register-level bare-metal driver and project templates targeting the **STM32C5 NUCLEO-C562RE** development board (Cortex-M33, `target/stm32u5x.cfg`). The GPIO and UART drivers are hand-coded against the STM32C5-compatible register map — no HAL, no CubeMX, no vendor device headers beyond CMSIS core (used only for NVIC interrupt control).

## Board & Hardware

| Item | Value |
|---|---|
| MCU | STM32C562RE (Cortex-M33, `cortex-m33` toolchain flag) |
| NUCLEO type | NUCLEO-C562RE |
| Flash base | `0x08000000` |
| RAM base | `0x20000000` |
| RCC base | `0x44020C00` |
| GPIO base | `0x42020000` (AHB2) |

### Onboard User LEDs (NUCLEO-C562RE)

| LED | Pin | Colour | Notes |
|---|---|---|---|
| LD1 | PA5 | Green | Only user LED; Arduino D13 |

## Folder Structure

```
STM32C5-NUCLEO-Projects/
├── Makefile                  # Build, flash, debug automation (all projects)
├── stm32c562re.icf           # IAR linker configuration (shared by all projects)
├── Drivers/
│   ├── CMSIS/                # CMSIS core headers (core_cm4.h — used for NVIC only)
│   ├── GPIO/                 # Register-level GPIO driver (STM32C5 port)
│   │   ├── inc/gpio.h        # RCC_TypeDef, GPIO_ManualTypeDef, pin macros
│   │   └── src/              # gpio.c, main.c, startup.c
│   ├── UART/                 # Register-level UART driver (STM32C5 port, USART v2)
│   │   ├── inc/uart.h        # USART_ManualType (ISR/RDR/TDR), IRQn_Type
│   │   └── src/              # uart.c, main.c, startup.c
│   └── bxCAN/                # FDCAN driver (STM32C5 port)
│       ├── inc/bxcan.h
│       └── src/              # bxcan.c, main.c, startup.c
└── Projects/                 # Application-level projects (use the Drivers above)
    └── LED_Blink/            # LED blink demo with medical/power LED patterns
```

## Architecture

Drivers follow **object-oriented C** style — each peripheral has a handle struct and a constructor:

```c
/* GPIO example */
GPIO_InitTypeDef led_init = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL,
                               .Speed = GPIO_SPEED_MEDIUM, .Pin = GPIO_PIN_7 };
GPIO_HandleTypeDef ld1;
GPIO_constructor(&ld1, GPIO_C, &led_init);   // enable clock + configure PC7
GPIO_TogglePin(&ld1, GPIO_PIN_7);            // toggle LD1 green

/* UART example */
USART_HandleType uart2;
USART_constructor(&uart2, USART_2, RX_AND_TX, __115200);
USART_WriteString(&uart2, "Hello\n");
```

CMSIS (`core_cm4.h`) is used **only** for `NVIC_SetPriority()` / `NVIC_EnableIRQ()`. All peripheral register access uses hand-defined structs (`RCC_TypeDef`, `GPIO_ManualTypeDef`, `USART_ManualType`) mapped directly to hardware addresses.

## Prerequisites

- **VS Code** (latest)
- **Cortex-Debug extension** (`marus25.cortex-debug`)
- **arm-none-eabi-gcc** toolchain (GCC 12+ recommended, add to PATH)
  - Linux: `sudo apt install gcc-arm-none-eabi`
  - Windows: download from [Arm Developer](https://developer.arm.com/downloads/-/gnu-rm)
- **OpenOCD** with STM32C5 target support (add to PATH)
- **ST-Link USB drivers** ([STSW-LINK009](https://www.st.com/en/development-tools/stsw-link009.html))
- **IAR Embedded Workbench for ARM** (optional, v9.70+ recommended)
  - Download from [IAR Systems](https://www.iar.com/downloads/)
  - A free evaluation license is sufficient for building these projects

## Building

### GCC (default)

```sh
# Build the GPIO driver demo (NUCLEO LED blink)
make build PROJECT_DIR=Drivers/GPIO

# Build the UART driver demo
make build PROJECT_DIR=Drivers/UART

# Build the bxCAN (FDCAN) driver demo
make build PROJECT_DIR=Drivers/bxCAN
```

Output files (`main.elf`, `main.bin`, `main.hex`, `main.map`) are generated inside the selected `PROJECT_DIR`.

### IAR Embedded Workbench

The IAR build uses `iccarm`, `ilinkarm`, and `ielftool` directly from the Makefile — no `.ewp` project file is needed. The same source lists, include paths, and defines used by the GCC build are automatically translated to IAR equivalents.

**Setup:**

1. Install IAR EWARM and note the installation path (default: `D:/iar/ewarm-9.70.4/`).
2. If your path differs, override it on the command line or in the Makefile:
   ```sh
   make iar-build PROJECT_DIR=Drivers/GPIO IAR_ROOT=C:/path/to/ewarm
   ```
3. A shared IAR linker configuration file `stm32c562re.icf` in the repository root defines the memory layout (Flash 1024 KB @ `0x08000000`, RAM 128 KB @ `0x20000000`).

**Build commands:**

```sh
# Build with IAR
make iar-build PROJECT_DIR=Drivers/GPIO
make iar-build PROJECT_DIR=Drivers/UART
make iar-build PROJECT_DIR=Drivers/bxCAN
make iar-build PROJECT_DIR=Projects/LED_Blink

# Clean IAR build artifacts
make iar-clean PROJECT_DIR=Drivers/GPIO
```

IAR output files are generated in `iar_out/` at the repository root (`main.out`, `main.bin`, `main.hex`, `main.map`).

**How it works:**

Each project's `startup.c` uses `#ifdef __ICCARM__` guards to provide both IAR and GCC startup code in a single file:

- **IAR path:** Uses `CSTACK$$Limit` (linker symbol from `.icf`), `#pragma location = ".intvec"` for the vector table, `#pragma weak` for IRQ handlers, and calls `__iar_data_init3()` for `.data`/`.bss` initialisation.
- **GCC path:** Uses `_estack` / `__etext` / `__bss_start__` etc. from the GCC `linker.ld`, `__attribute__((section(".isr_vector")))`, and manual `.data` copy / `.bss` zero-fill.

**Available IAR make targets:**

| Target | Description |
|---|---|
| `iar-build` | Compile + link + generate `.bin` with IAR tools |
| `iar-clean` | Remove IAR build artifacts (`iar_out/`) |
| `iar-flash` | Build with IAR and flash `iar_out/main.bin` |
| `iar-debug` | Build with IAR in debug mode (`DEBUG=1`), flash, then attach with VS Code `cspy` launch |

**Overridable variables:**

| Variable | Default | Description |
|---|---|---|
| `IAR_ROOT` | `D:/iar/ewarm-9.70.4` | Root path of EWARM installation |
| `IAR_ICF` | `stm32c562re.icf` | IAR linker configuration file |
| `DEBUG` | `0` | Use `1` for debugger-friendly codegen (`make debug`, `make iar-debug`) |

## Flashing

Connect the NUCLEO board via USB (ST-Link), then:

```sh
make flash PROJECT_DIR=Drivers/GPIO
make flash PROJECT_DIR=Drivers/UART
```

## Debugging in VS Code

### GCC + Cortex-Debug (OpenOCD / ST-Link)

1. Ensure `.vscode/launch.json` targets Cortex-Debug with OpenOCD and ST-Link
2. Press `F5` or open **Run and Debug**
3. Set breakpoints and step through code normally

### IAR C-SPY Debugger

IAR ships its own debugger, **C-SPY**, which supports ST-Link out of the box. There are two practical ways in this repository:

#### Option A: From the command line (recommended for quick tests)

1. Build with IAR:
   ```sh
   make iar-build PROJECT_DIR=Drivers/GPIO
   ```
2. Launch **C-SPY Batch** (`cspybat.exe`) to flash and run:
   ```sh
   "D:/iar/ewarm-9.70.4/common/bin/cspybat.exe" ^
     --backend "D:/iar/ewarm-9.70.4/arm/bin/armproc.dll" ^
     "D:/iar/ewarm-9.70.4/arm/bin/armstlink2.dll" ^
   iar_out/main.out ^
     --plugin "D:/iar/ewarm-9.70.4/arm/bin/armbat.dll" ^
     --stlink_interface SWD
   ```
   This downloads the firmware, starts the CPU, and streams semihosting output (if any) to the terminal. Press `Ctrl+C` to stop.

#### Option B: VS Code with IAR C-SPY extension (recommended)

1. Build + flash debug firmware:
   ```sh
   make iar-debug PROJECT_DIR=Drivers/GPIO
   ```
2. Start the VS Code debug config **IAR C-SPY: ST-LINK** (`F5`).
3. Ensure `program` points to `iar_out/main.out` and `workbenchPath` points to your installed EWARM root.
4. If step controls are disabled, press **Pause** first; step buttons activate only when target is halted.
5. If breakpoints are skipped, confirm you built the same `PROJECT_DIR` you are editing and run `make iar-debug` again.

#### Option C: GCC + Cortex-Debug in VS Code

Use Cortex-Debug with OpenOCD for GCC-built firmware.

1. Build + flash debug firmware:
   ```sh
   make debug PROJECT_DIR=Drivers/GPIO
   ```
2. Ensure `.vscode/launch.json` contains the Cortex configuration:
   ```jsonc
   {
     "name": "Cortex Debug: ST-LINK",
     "type": "cortex-debug",
     "request": "launch",
     "executable": "${workspaceFolder}/build_out/main.elf",
     "runToEntryPoint": "main"
   }
   ```
3. Select **Cortex Debug: ST-LINK** in Run and Debug and press `F5`.

## Key Register Addresses (STM32C5)

| Peripheral | Base Address | Bus |
|---|---|---|
| RCC | `0x46020C00` | — |
| GPIOA | `0x42020000` | AHB2 |
| GPIOB | `0x42020400` | AHB2 |
| GPIOC | `0x42020800` | AHB2 |
| GPIOG | `0x42021800` | AHB2 |
| USART1 | `0x40013800` | APB2 |
| USART2 | `0x40004400` | APB1 |

## Troubleshooting

- **Missing separator in Makefile** — ensure command lines use tabs, not spaces
- **Toolchain not found** — add `arm-none-eabi-gcc` and `openocd` to system PATH
- **Wrong LED pins** — check your board's user manual and update `Drivers/GPIO/src/main.c`
- **OpenOCD target not found** — ensure your OpenOCD installation includes `target/stm32u5x.cfg` (used by this repo's STM32C5 setup)
- **Breakpoints never hit** — most common causes:
   - Built a different project than the file you are editing. Check `PROJECT_DIR` in `Makefile` or pass `PROJECT_DIR=...` on the command.
   - Optimized build loaded. Use `make debug` or `make iar-debug` (both enforce debug-friendly options).
   - Breakpoint is hollow/unverified. Start debug first, then check it turns solid red.

## References

- [Cortex-Debug Extension](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
- [OpenOCD Documentation](http://openocd.org/doc/html/index.html)
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [IAR Embedded Workbench](https://www.iar.com/ewarm) — compiler, linker, and C-SPY debugger

