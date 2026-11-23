#!/bin/sh
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root="$here/debug/device/db1-qualification-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$here/debug/device"
mkdir "$root"
DYNA_R36S_PROFILE_DIR="$root" sh "$here/debug/collect.sh" "$root/system" >/dev/null 2>&1 || true

unset DYNA_R36S_CONSOLE DYNA_GOODLUCKOS_REFERENCE DYNA_GOODLUCKOS_CANDIDATE DYNA_DB1_POLL_TAILS
export DYNA_GOODLUCKOS_VSYNC=0 GENESIS_HOST_TIMING_DUMP=1
export GENESIS_HOST_SWAP_TIMING=1
export GENESIS_DISPLAY_TRACE=1 GENESIS_AUDIO_BUFFER_TRACE=1
export GENESIS_R36S_TRACE_RASTER_FRAME=6070

run() {
    mode=$1
    shift
    export DYNA_R36S_PROFILE_DIR="$root/$mode"
    mkdir "$DYNA_R36S_PROFILE_DIR"
    printf '%s\n' 'vsync = 2' > "$DYNA_R36S_PROFILE_DIR/dynabrothers-video.ini"
    result=0
    sh "$here/launch.sh" 1 --measure-runtime 6200 --input-script "$here/debug/normal1.input" "$@" || result=$?
    printf '%s exit=%s\n' "$mode" "$result" >> "$root/status.txt"
    return "$result"
}

result=0
if [ -f "$here/DynaBrothers1Reference" ]; then
    DYNA_GOODLUCKOS_REFERENCE=1 run reference-paced || result=1
fi
if [ -f "$here/DynaBrothers1Qualification" ]; then
    export DYNA_GOODLUCKOS_CANDIDATE=1 DYNA_DB1_POLL_TAILS=1
fi
run paced || result=1
run uncapped --runtime-uncapped || result=1
exit "$result"
