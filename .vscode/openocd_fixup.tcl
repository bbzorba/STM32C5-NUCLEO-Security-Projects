# openocd_fixup.tcl — Board-independent OpenOCD event handler fix
# =========================================================================
# PROBLEM (why this file exists)
#
#   Zephyr board configs set "gdb_breakpoint_override hard" inside the
#   gdb-attach event.  This forces EVERY breakpoint to consume a hardware
#   FPB comparator (STM32F407 has 6; nRF52840 has 8; ESP32 has 2/core).
#
#   If Cortex-Debug LiveWatch is enabled, TWO GDB connections are opened.
#   Both trigger gdb-attach → both call "reset halt" → the second resets
#   the CPU mid-session.  Both call "gdb_breakpoint_override hard" →
#   FPB comparators are exhausted.
#
# FIX
#   1. Track GDB connection count.  Only "reset halt" on the FIRST
#      connection.  Secondary connections attach silently.
#
#   2. Use "gdb_breakpoint_override hard" (hardware FPB breakpoints) —
#      fast, reliable, no flash sector rewrite required.
#      Limit: 6 breakpoints on STM32F407 (sufficient for normal debugging).
#
#   3. Only resume on the LAST disconnect.
#
# BOARD-INDEPENDENCE
#   "foreach t [target names]" iterates whatever targets the board config
#   created.  Works for any board without changes.
# =========================================================================

set __dbg_conn 0

foreach t [target names] {
    $t configure -event gdb-attach {
        global __dbg_conn
        incr __dbg_conn
        if {$__dbg_conn == 1} {
            echo "Debugger attaching: halting target (first connection)"
            reset halt
            gdb_breakpoint_override hard
        } else {
            echo "Debugger attaching: secondary connection #$__dbg_conn (ignored)"
        }
    }

    $t configure -event gdb-detach {
        global __dbg_conn
        incr __dbg_conn -1
        if {$__dbg_conn <= 0} {
            set __dbg_conn 0
            echo "All debuggers detached: resuming target"
            resume
        } else {
            echo "Debugger detached: $__dbg_conn connection(s) still active"
        }
    }
}

