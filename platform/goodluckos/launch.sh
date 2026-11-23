#!/bin/sh
set -eu

case "${1:-}" in
    1|2) game=$1; shift ;;
    *) exit 2 ;;
esac

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"
rom="$here/roms/db$game.md"
program="$here/DynaBrothers${game}AOT"
if [ "$game" = 1 ] && [ "${DYNA_GOODLUCKOS_REFERENCE:-0}" = 1 ]; then
    program="$here/DynaBrothers1Reference"
fi
export GENESIS_USER_DIR="${DYNA_R36S_PROFILE_DIR:-$here/user}"
mkdir -p "$GENESIS_USER_DIR"
if [ "${DYNA_R36S_CONSOLE:-}" != 1 ]; then
    exec >"$GENESIS_USER_DIR/db$game-last-run.log" 2>&1
fi

if [ ! -r "$rom" ]; then
    printf 'Copy your cartridge dump to %s\n' "$rom" >&2
    exit 5
fi

# Keep the firmware's gamepad mapping even when appd skips login profiles.
if [ -z "${SDL_GAMECONTROLLERCONFIG:-}" ] && [ -r /etc/profile.d/sdl_controller.sh ]; then
    . /etc/profile.d/sdl_controller.sh
fi
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
if [ "${DYNA_GOODLUCKOS_VSYNC:-1}" = 0 ]; then
    export GENESIS_NO_VSYNC=1
fi
exec "$program" --no-launcher --fullscreen --widescreen off "$rom" "$@"
