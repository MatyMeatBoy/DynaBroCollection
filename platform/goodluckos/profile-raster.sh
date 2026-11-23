#!/bin/sh
set -eu

case "${1:-}" in
    start) frame=1200; frames=1230 ;;
    battle) frame=6070; frames=6200 ;;
    *) exit 2 ;;
esac

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export DYNA_R36S_PROFILE_DIR="$here/debug/device/db1-raster-$1"
export DYNA_GOODLUCKOS_VSYNC=0
export GENESIS_HOST_TIMING_DUMP=1
export GENESIS_R36S_TRACE_RASTER_FRAME="$frame"
exec sh "$here/launch.sh" 1 --measure-runtime "$frames" --input-script "$here/debug/normal1.input"
