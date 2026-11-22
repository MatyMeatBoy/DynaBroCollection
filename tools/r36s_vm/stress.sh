#!/bin/sh
set -eu
cd /run/r36s-lab
read frames cpu_quota memory_mib game < stress-settings
case "$game" in
    db1) launcher="./Dyna Brothers.sh" ;;
    db2) launcher="./Dyna Brothers 2 Special.sh" ;;
    *) exit 2 ;;
esac
test -r "dynabrocollection/roms/$game.md"
root=/sys/fs/cgroup
mount -t cgroup2 none "$root"
printf '+cpu +memory\n' > "$root/cgroup.subtree_control"
export GENESIS_HOST_TIMING_DUMP=1

run_case() {
    name=$1
    quota=$2
    limit=$3
    video=$4
    renderer=$5
    injection=${6:-}
    group="$root/r36s-$name"
    mkdir "$group"
    printf '%s 100000\n' "$quota" > "$group/cpu.max"
    printf '%s\n' "$limit" > "$group/memory.max"
    printf 'SCENARIO_BEGIN %s\n' "$name"
    grep '^MemTotal:' /proc/meminfo
    printf 'cpu.max '; cat "$group/cpu.max"
    printf 'memory.max '; cat "$group/memory.max"
    set +e
    set --
    if [ -f scenario.input ]; then set -- --input-script /run/r36s-lab/scenario.input; fi
    SDL_VIDEO_DRIVER=$video SDL_RENDER_DRIVER=$renderer \
        sh -c 'printf "%s\n" "$$" > "$1/cgroup.procs" || exit 125; shift; exec "$@"' \
        scenario "$group" env LD_PRELOAD=/run/r36s-lab/sdl-fault.so "R36S_SDL_FAULT=$injection" \
        "$launcher" --measure-runtime "$frames" --window-size 640x480 "$@"
    result=$?
    set -e
    printf 'SCENARIO_EXIT %s\n' "$result"
    cat "$group/cpu.stat"
    printf 'memory.peak '; cat "$group/memory.peak"
    cat "$group/memory.events"
    if [ -d dynabrocollection/user ]; then
        mv dynabrocollection/user "profile-$name"
    fi
    printf 'SCENARIO_END %s\n' "$name"
}

run_case software max max dummy software
run_case cpu_limited "$cpu_quota" max dummy software
run_case memory_limited max "$((memory_mib * 1048576))" dummy software
run_case missing_drm max max kmsdrm opengles2
run_case missing_renderer max max dummy r36s_unavailable
run_case slow_present max max dummy software present
run_case upload_failure max max dummy software upload
echo DARKOS_STRESS_DONE
