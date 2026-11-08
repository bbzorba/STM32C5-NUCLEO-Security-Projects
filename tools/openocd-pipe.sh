#!/usr/bin/env bash
# GDB target for VS Code cortex-debug (launch.json "gdbTarget"):
#   target extended-remote | tools/openocd-pipe.sh
# OpenOCD speaks the GDB protocol over stdin/stdout (gdb_port pipe), so no TCP
# port is opened, and from WSL the Windows openocd.exe can be used — the
# ST-LINK stays on Windows, next to STM32CubeProgrammer. GDB ends OpenOCD when
# the session ends, which frees the ST-LINK again. Pause (interrupt) needs
# "mi-async on", which cortex-debug always sets.
#
# OpenOCD: $OPENOCD, else openocd / openocd.exe on PATH, else PlatformIO's.
# Its log goes to build/openocd.log.
set -euo pipefail
cd "$(dirname "$0")/.."

ocd=${OPENOCD:-$(command -v openocd || command -v openocd.exe || true)}
if [ -z "$ocd" ]; then
    for c in /mnt/c/Users/*/.platformio/packages/tool-openocd/bin/openocd.exe; do
        [ -f "$c" ] && ocd=$c && break
    done
fi
if [ -z "$ocd" ]; then
    echo "openocd-pipe.sh: OpenOCD not found — install it or set OPENOCD" >&2
    exit 1
fi

# A Windows openocd.exe needs Windows paths (F:/… — backslashes are escapes in Tcl)
path() { case "$ocd" in *.exe) wslpath -m "$1" ;; *) echo "$1" ;; esac; }

# xPack / PlatformIO layout: bin/openocd(.exe) next to openocd/scripts
scripts=$(dirname "$(dirname "$ocd")")/openocd/scripts
search=()
[ -d "$scripts" ] && search=(-s "$(path "$scripts")")

mkdir -p build
args=("${search[@]}"
      -c "log_output {$(path "$PWD/build/openocd.log")}"   # {…}: Tcl quoting, the path may contain spaces
      -c "telnet_port disabled" -c "tcl_port disabled"
      -c "set CPUTAPID 0x6ba02477"
      -f interface/stlink-dap.cfg -c "transport select dapdirect_swd"
      -f "$(path stm32c562re.cfg)")

exec "$ocd" "${args[@]}" -c "gdb_port pipe"
