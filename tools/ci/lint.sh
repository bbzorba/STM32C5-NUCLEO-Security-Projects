#!/usr/bin/env bash
# Static checks run by CI (.github/workflows/ci.yml), also usable locally:
#   tools/ci/lint.sh            # cppcheck + ruff
# Needs cppcheck and ruff on PATH (CI: apt install cppcheck, pip install ruff).
set -euo pipefail
cd "$(dirname "$0")/../.."

# Our C code: every module folder (vendor submodules have no module.mk)
modules=$(for f in Drivers/*/module.mk Projects/*/module.mk; do dirname "$f"; done)
includes=$(for m in $modules; do [ -d "$m/inc" ] && echo "-I$m/inc"; done)

# On GitHub Actions, report findings as annotations on the commit / pull request
cppcheck_fmt=() ruff_fmt=()
if [ "${GITHUB_ACTIONS:-}" = true ]; then
    cppcheck_fmt=(--template='::error file={file},line={line},col={column},title=cppcheck {id}::{message}')
    ruff_fmt=(--output-format=github)
fi

echo "=== cppcheck $(cppcheck --version | awk '{print $2}') ==="
# Suppressed checks:
#   comparePointers in startup.c  - linker-symbol copy loops (&__data_start__ .. &__data_end__)
#   invalidPrintfArgType_uint     - cppcheck models uint32_t as unsigned int; on arm-none-eabi
#                                   it is unsigned long. GCC -Wformat (in -Wall) checks formats
#                                   with the real types during the build.
# shellcheck disable=SC2086
cppcheck --quiet --std=c11 --platform=unix32 \
    --enable=warning,performance,portability \
    --inline-suppr --error-exitcode=1 "${cppcheck_fmt[@]}" \
    --suppress=missingIncludeSystem \
    --suppress='comparePointers:*/startup.c' \
    --suppress=invalidPrintfArgType_uint \
    -DSTM32C5xx -IDrivers/compat_inc $includes \
    $modules 2>&1

echo "=== ruff $(ruff --version | awk '{print $2}') ==="
ruff check --target-version py38 "${ruff_fmt[@]}" tools/
echo "lint OK"
