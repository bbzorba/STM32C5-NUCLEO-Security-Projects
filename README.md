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
├── Drivers/
│   ├── CMSIS/                # CMSIS core headers (core_cm4.h — used for NVIC only)
│   ├── GPIO/                 # Register-level GPIO driver (STM32C5 port)
│   │   ├── inc/gpio.h        # RCC_TypeDef, GPIO_ManualTypeDef, pin macros
│   │   └── src/              # gpio.c, main.c, startup.c, system_stm32f4xx.c
│   └── UART/                 # Register-level UART driver (STM32C5 port, USART v2)
│       ├── inc/uart.h        # USART_ManualType (ISR/RDR/TDR), IRQn_Type
│       └── src/              # uart.c, main.c, startup.c, system_stm32f4xx.c
└── Projects/                 # Application-level projects (use the Drivers above)
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

## Building

```sh
# Build the GPIO driver demo (NUCLEO LED blink)
make build PROJECT_DIR=Drivers/GPIO

# Build the UART driver demo
make build PROJECT_DIR=Drivers/UART
```

Output files (`main.elf`, `main.bin`, `main.hex`, `main.map`) are generated inside the selected `PROJECT_DIR`.

## Flashing

Connect the NUCLEO board via USB (ST-Link), then:

```sh
make flash PROJECT_DIR=Drivers/GPIO
make flash PROJECT_DIR=Drivers/UART
```

## Debugging in VS Code

1. Ensure `.vscode/launch.json` targets Cortex-Debug with OpenOCD and ST-Link
2. Press `F5` or open **Run and Debug**
3. Set breakpoints and step through code normally

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
- **OpenOCD target not found** — ensure your OpenOCD installation includes `target/stm32c5x.cfg`

## References

- [Cortex-Debug Extension](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
- [OpenOCD Documentation](http://openocd.org/doc/html/index.html)
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/gnu-rm)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)

