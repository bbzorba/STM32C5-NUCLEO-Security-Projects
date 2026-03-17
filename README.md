# STM32F407G-DISC1 Bare-Metal Projects (VS Code)

This project provides a minimal bare-metal template for the STM32F407G-DISC1 (STM32F4-DISCO) board, using arm-none-eabi-gcc, OpenOCD, and VS Code with Cortex-Debug. It is designed for register-level development (no HAL, no CubeMX) and supports reliable build, flash, and debug workflows.

## Folder Structure
```
STM32_Project/
├── linker.ld                # Linker script for STM32F407
├── main.bin                 # Compiled binary (output)
├── main.elf                 # Compiled ELF (output)
├── main.hex                 # Compiled HEX (output)
├── main.map                 # Linker map (output)
├── Makefile                 # Build, flash, and debug automation
├── .vscode/
│   ├── launch.json          # VS Code debug configuration (Cortex-Debug)
│   ├── openocd.cfg          # OpenOCD configuration (optional)
│   └── tasks.json           # VS Code build/flash tasks (optional)
├── inc/
│   ├── core_cm4.h           # CMSIS core header
│   ├── stm32f407xx.h        # STM32F407 device header
│   ├── stm32f4xx.h          # STM32F4 family header
│   └── system_stm32f4xx.h   # System header
├── src/
│   ├── main.c               # Main application (LED blink, etc.)
│   ├── startup.c            # Startup code, vector table
│   └── system_stm32f4xx.c   # SystemInit stub
```

## Prerequisites
- **VS Code** (latest recommended)
- **Cortex-Debug extension** (for debugging)
- **arm-none-eabi-gcc toolchain** (compiler, linker, objcopy, size)
- **OpenOCD** (for flashing/debugging)
- **ST-Link USB drivers** (for STM32F4-DISCO)

## Installation & Setup
1. **Install VS Code**
   - Download and install from [https://code.visualstudio.com/](https://code.visualstudio.com/)

2. **Install Cortex-Debug Extension**
   - In VS Code, go to Extensions (`Ctrl+Shift+X`)
   - Search for `Cortex-Debug` and install

3. **Install arm-none-eabi-gcc Toolchain**
   - Download from [Arm Developer](https://developer.arm.com/downloads/-/gnu-rm)
   - Download via the terminal (if using linux PC) using this command:
         sudo apt install gcc-arm-none-eabi 
   - Add toolchain binaries to your system PATH

4. **Install OpenOCD**
   - Download from [https://openocd.org/](https://openocd.org/)
   - Add OpenOCD to your system PATH

5. **Install ST-Link USB Drivers**
   - Download from STMicroelectronics [STSW-LINK009](https://www.st.com/en/development-tools/stsw-link009.html)

6. **Clone or Copy Project Folder**
   - Place the project folder anywhere on your system

## Building the Project
- Open a terminal in the project folder where the Makefile is
- Run:
  ```
  make
  ```
- Output files (`main.elf`, `main.bin`, `main.hex`, `main.map`) will be generated in the project root

## Flashing the Board
- Connect the STM32F4-DISCO board via USB (ST-Link)
- Run:
  ```
  make flash
  ```
- This uses OpenOCD to program the board with `main.elf`

## Debugging in VS Code
1. **Configure Debugger**
   - Ensure `.vscode/launch.json` is present and set for Cortex-Debug with OpenOCD and ST-Link
   - Example config is provided in the project

2. **Start Debugging**
   - Press `F5` or click `Run and Debug` in VS Code
   - Set breakpoints, watch variables, step through code

## Customization
- Edit source files in `src/` and headers in `inc/` as needed for your application
- The project is generic for STM32F407G-DISC1, but can be adapted for other STM32F4 devices by updating headers and linker script

## Troubleshooting
- **Missing separator error in Makefile:** Ensure all command lines are indented with tabs, not spaces
- **Toolchain not found:** Add arm-none-eabi-gcc and OpenOCD to your system PATH
- **Debugging issues:** Confirm ST-Link drivers are installed and board is connected
- **Breakpoints not working:** Ensure debug info (`-g` flag) is enabled in Makefile and startup code is correct

## References
- [Cortex-Debug Extension](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
- [OpenOCD Documentation](http://openocd.org/doc/html/index.html)
- [STM32F407G-DISC1 User Manual](https://www.st.com/resource/en/user_manual/dm00039084.pdf)
- [Arm GNU Toolchain](https://developer.arm.com/downloads/-/gnu-rm)

---
This template is intended for register-level STM32F4-DISCO development in VS Code. For CubeMX/HAL projects, use STM32CubeIDE or adapt the build system accordingly.
