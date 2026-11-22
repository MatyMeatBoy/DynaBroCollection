#!/bin/bash

set -u
game=$1
shift
case "$game" in
    1|2) ;;
    *) exit 2 ;;
esac
GAMEDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
cd "$GAMEDIR" || exit 1
ports=$(dirname -- "$GAMEDIR")
rom="$GAMEDIR/roms/db$game.md"
program="$GAMEDIR/DynaBrothers${game}AOT"

# PortMaster can supply the device's SDL mapping. Standalone installs also work
# with SDL's built-in mapping and the bundled controller database.
set +u
for controlfolder in /opt/system/Tools/PortMaster /opt/tools/PortMaster \
    "${XDG_DATA_HOME:-$HOME/.local/share}/PortMaster" "$ports/PortMaster" \
    /roms/ports/PortMaster /roms2/ports/PortMaster; do
    [ -r "$controlfolder/control.txt" ] || continue
    . "$controlfolder/control.txt"
    if command -v get_controls >/dev/null 2>&1; then
        get_controls
        if [ -z "${SDL_GAMECONTROLLERCONFIG:-}" ] && [ -n "${sdl_controllerconfig:-}" ]; then
            export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
        fi
    fi
    break
done
set -u

mkdir -p "$GAMEDIR/user" || exit 1
if [ "${DYNA_R36S_CONSOLE:-}" != 1 ]; then
    exec >"$GAMEDIR/user/db$game-last-run.log" 2>&1
fi
if [ ! -r "$rom" ]; then
    message="Copy your supported cartridge dump to dynabrocollection/roms/db$game.md."
    if command -v pm_message >/dev/null 2>&1; then pm_message "$message"; fi
    printf '%s\n' "$message" >&2
    exit 5
fi

export GENESIS_USER_DIR="$GAMEDIR/user"
export LD_LIBRARY_PATH="$GAMEDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-kmsdrm}"
export SDL_AUDIO_DRIVER="${SDL_AUDIO_DRIVER:-alsa}"

if command -v pm_platform_helper >/dev/null 2>&1; then pm_platform_helper "$program"; fi
"$program" --no-launcher --fullscreen --widescreen off "$rom" "$@"
result=$?
if command -v pm_finish >/dev/null 2>&1; then pm_finish; fi
exit "$result"
