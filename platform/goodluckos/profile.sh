#!/bin/sh
set -eu

game=${1:-}
mode=${2:-}
case "$game" in 1|2) ;; *) exit 2 ;; esac
case "$mode" in
    sync) unset GENESIS_NO_VSYNC; export DYNA_GOODLUCKOS_VSYNC=1 ;;
    paced) export DYNA_GOODLUCKOS_VSYNC=0 ;;
    *) exit 2 ;;
esac

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export DYNA_R36S_PROFILE_DIR="$here/debug/device/db$game-$mode"
export GENESIS_HOST_TIMING_DUMP=1
export GENESIS_DISPLAY_TRACE=1
mkdir -p "$DYNA_R36S_PROFILE_DIR"
if [ "$mode" = paced ]; then
    printf '%s\n' 'vsync = 2' > "$DYNA_R36S_PROFILE_DIR/dynabrothers-video.ini"
else
    printf '%s\n' 'vsync = 1' > "$DYNA_R36S_PROFILE_DIR/dynabrothers-video.ini"
fi
if [ "$game" = 1 ]; then
    set -- --measure-runtime 6200 --input-script "$here/debug/normal1.input"
else
    set -- --measure-runtime 3600
fi
exec sh "$here/launch.sh" "$game" "$@"
