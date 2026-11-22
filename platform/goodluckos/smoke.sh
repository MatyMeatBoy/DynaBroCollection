#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    printf 'Usage: %s FIRMWARE_ROOT ARMV7_STAGE\n' "$0" >&2
    exit 2
fi

root=$(CDPATH= cd -- "$1" && pwd)
stage=$(CDPATH= cd -- "$2" && pwd)
qemu=${QEMU_ARM:-qemu-arm}
loader="$root/lib/ld-linux-armhf.so.3"
if [ ! -r "$loader" ]; then
    printf 'Missing GoodLuckOS ARM hard-float loader: %s\n' "$loader" >&2
    exit 2
fi

unset LD_LIBRARY_PATH LD_PRELOAD
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
for game in DynaBrothers1AOT DynaBrothers2AOT; do
    printf '\n%s\n' "$game"
    "$qemu" -cpu cortex-a7 -L "$root" "$loader" --list "$stage/$game"
    "$qemu" -cpu cortex-a7 -L "$root" "$stage/$game" --build-info
done
