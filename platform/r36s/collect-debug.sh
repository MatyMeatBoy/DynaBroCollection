#!/bin/sh

# Collect an explicit, local diagnostic bundle for a packaged port.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game=$(CDPATH= cd -- "$here/.." && pwd)
profile=${DYNA_R36S_PROFILE_DIR:-"$game/user"}
out=${1:-"$game/debug-$(date +%Y%m%d-%H%M%S)"}
mkdir "$out"
out=$(CDPATH= cd -- "$out" && pwd)

{
    uname -srm
    getconf LONG_BIT
    getconf GNU_LIBC_VERSION || true
    if [ -r /etc/os-release ]; then cat /etc/os-release; fi
    if [ -r /proc/device-tree/model ]; then
        tr -d '\000' </proc/device-tree/model
        printf '\n'
    fi
    grep -E '^(MemTotal|MemAvailable):' /proc/meminfo || true
    for item in /sys/class/drm/card*-*/status /sys/class/drm/card*-*/modes \
                /sys/devices/system/cpu/cpufreq/policy*/scaling_governor \
                /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq \
                /sys/class/thermal/thermal_zone*/temp /proc/asound/cards; do
        if [ -r "$item" ]; then
            printf '\n%s\n' "$item"
            cat "$item"
        fi
    done
    # Linux input capabilities describe devices without reading live events.
    if [ -r /proc/bus/input/devices ]; then
        awk 'BEGIN{RS="";ORS="\n\n"} /Handlers=.*js[0-9]/' /proc/bus/input/devices |
            grep -Ev '^(P: Phys|U: Uniq)' || true
    fi
} > "$out/linux.txt" 2>&1

dmesg 2>/dev/null | grep -Ei 'drm|panfrost|mali|alsa|snd|thermal|oom-kill|out of memory' |
    tail -150 > "$out/kernel-graphics-audio.log" || true
for item in "$profile/db1-last-run.log" "$profile/db2-last-run.log"; do
    if [ -f "$item" ]; then cp "$item" "$out/"; fi
done
printf '%s\n' "$out"
