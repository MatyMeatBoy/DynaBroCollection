"""Run a dArkOS disk image in QEMU without modifying the source image."""

import argparse
import gzip
import io
import json
import os
from pathlib import Path
import re
import shutil
import socket
import stat
import subprocess
import sys
import tarfile
import threading
import time
import zipfile

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[1]
HERE = ROOT / "local" / "darkos-vm"
sys.path.insert(0, str(ROOT))


def cpio(out, path, mode, data=b""):
    name = path.encode() + b"\0"
    fields = (0, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(name), 0)
    out.write(b"070701" + "".join(f"{value:08x}" for value in fields).encode())
    out.write(name + bytes((-(110 + len(name))) % 4))
    out.write(data + bytes(-len(data) % 4))


INIT = b"""#!/usr/bin/busybox sh
set -eu
export PATH=/usr/bin:/usr/sbin:/bin:/sbin
B=/usr/bin/busybox
$B mount -t proc proc /proc
$B mount -t sysfs sysfs /sys
$B mount -t devtmpfs devtmpfs /dev
$B mkdir -p /disk
depmod -a || true
modprobe virtio_mmio || true
modprobe virtio_blk || true
if [ -f /lab/sdl-preview.so ]; then modprobe virtio_console; fi
if ! modprobe btrfs; then
    $B dmesg | $B tail -40
    $B poweroff -f
fi
$B sleep 1
$B cat /proc/partitions
if $B test -e /vm-probe-only; then
    $B poweroff -f
fi
for part in /dev/vda[1-9]; do
    if $B mount -t btrfs -o ro,nologreplay "$part" /disk; then
        if [ -x /disk/bin/sh ] && [ -r /disk/etc/os-release ]; then
            echo DARKOS_ROOT_PARTITION="$part"
            break
        fi
        $B umount /disk
    fi
done
if ! test -x /disk/bin/sh; then
    $B dmesg | $B tail -30
    $B poweroff -f
fi
$B mount -t tmpfs tmpfs /disk/run
$B mount --bind /dev /disk/dev
$B mount --bind /proc /disk/proc
$B mount --bind /sys /disk/sys
$B cp -a /lab /disk/run/r36s-lab
if [ -d /lab/units ]; then
    $B mkdir -p /disk/run/systemd/system
    $B cp /lab/units/* /disk/run/systemd/system/
    exec $B switch_root /disk /sbin/init
fi
exec $B switch_root /disk /bin/sh /run/r36s-lab/check.sh
"""

CHECK = b"""#!/bin/sh
set -u
printf '\\n--- dArkOS userspace ---\\n'
cat /etc/os-release
uname -m
getconf LONG_BIT
getconf GNU_LIBC_VERSION
printf 'PID 1: '
cat /proc/1/comm
printf '\\n--- Firmware files ---\\n'
for path in /usr/bin/emulationstation /opt/emulationstation /opt/system /usr/local/bin; do
    ls -ld "$path" 2>/dev/null || true
done
cd /run/r36s-lab
export HOME=/run/r36s-lab
export DYNA_R36S_CONSOLE=1 SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy SDL_RENDER_DRIVER=software
if [ -f /run/r36s-lab/sdl-preview.so ]; then
    for port in /sys/class/virtio-ports/*; do
        if [ "$(cat "$port/name" 2>/dev/null)" = org.dynabro.preview ]; then
            export R36S_PREVIEW_PORT="/dev/${port##*/}"
            break
        fi
    done
    test -n "${R36S_PREVIEW_PORT:-}" || exit 1
    test -r dynabrocollection/roms/db1.md || exit 1
    if [ -f /run/r36s-lab/sdl-preview-original.so ]; then
        export GENESIS_HOST_TIMING_DUMP=1
        echo PREVIEW_BASELINE
        LD_PRELOAD=/run/r36s-lab/sdl-preview-original.so "./Dyna Brothers.sh" --window-size 640x480 --measure-runtime 240 --runtime-uncapped || exit 1
        mv dynabrocollection/user preview-baseline-user
        echo PREVIEW_OPTIMIZED
        LD_PRELOAD=/run/r36s-lab/sdl-preview.so "./Dyna Brothers.sh" --window-size 320x240 --measure-runtime 240 --runtime-uncapped || exit 1
        echo PREVIEW_BENCH_OK
    else
        export GENESIS_NO_VSYNC=1
        LD_PRELOAD=/run/r36s-lab/sdl-preview.so "./Dyna Brothers.sh" --window-size 320x240
    fi
    exec /sbin/poweroff -f --no-wtmp
fi
if [ -f /run/r36s-lab/interactive ]; then
    echo DARKOS_SHELL_READY
    exec /bin/bash --noprofile --norc -i
fi
if [ -x /run/r36s-lab/stress.sh ]; then
    /run/r36s-lab/stress.sh || echo DARKOS_STRESS_FAILED
    exec /sbin/poweroff -f --no-wtmp
fi
if [ -x ./dynabrocollection/DynaBrothers1AOT ]; then
    for i in 1 2; do
        LD_LIBRARY_PATH="$PWD/dynabrocollection/lib" "./dynabrocollection/DynaBrothers${i}AOT" --build-info || exit 1
    done
    echo DARKOS_PORT_BINARIES_OK
    if [ -f dynabrocollection/roms/db1.md ]; then
        "./Dyna Brothers.sh" --benchmark 300 || exit 1
        echo DARKOS_DB1_OK
    fi
    if [ -f dynabrocollection/roms/db2.md ]; then
        "./Dyna Brothers 2 Special.sh" --benchmark 300 || exit 1
        echo DARKOS_DB2_OK
    fi
fi
echo DARKOS_USERSPACE_OK
exec /sbin/poweroff -f --no-wtmp
"""


USERSPACE_INIT = b"""#!/usr/bin/busybox sh
set -eu
export PATH=/usr/bin:/usr/sbin:/bin:/sbin
B=/usr/bin/busybox
$B mount -t proc proc /proc
$B mount -t sysfs sysfs /sys
$B mount -t devtmpfs devtmpfs /dev
$B mkdir -p /disk /lab/dynabrocollection/user
depmod -a
modprobe virtio_pci
modprobe virtio_blk
modprobe virtio_console
modprobe btrfs
$B sleep 1
$B mount -t btrfs -o ro,nologreplay /dev/vda2 /disk
test -r /disk/lib/ld-linux-aarch64.so.1
$B cat /disk/etc/os-release
for port in /sys/class/virtio-ports/*; do
    if [ "$($B cat "$port/name")" = org.dynabro.preview ]; then
        export R36S_PREVIEW_PORT="/dev/${port##*/}"
        break
    fi
done
test -n "${R36S_PREVIEW_PORT:-}"
cd /lab/dynabrocollection
test -r roms/db1.md
export GENESIS_USER_DIR="$PWD/user" SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy SDL_RENDER_DRIVER=software
if [ -f /lab/virtual-pad ]; then export R36S_PREVIEW_PAD=1; fi
if [ -f /lab/preview-60 ]; then export R36S_PREVIEW_FULL_RATE=1; fi
if [ -f /lab/preview-bench ]; then
    read frames < /lab/preview-bench
    export GENESIS_HOST_TIMING_DUMP=1
    set -- --measure-runtime "$frames"
    if [ -f /lab/bench-paced ]; then
        mkdir -p "$GENESIS_USER_DIR"
        printf '%s\n' 'vsync = 2' > "$GENESIS_USER_DIR/dynabrothers-video.ini"
        export R36S_PREVIEW_DEFER_VSYNC=1
    else
        set -- "$@" --runtime-uncapped
    fi
    if [ -f /lab/bench.input ]; then
        set -- "$@" --input-script /lab/bench.input
    fi
else
    set --
fi
/lab/qemu-aarch64 -cpu cortex-a35 -L /disk -E "LD_LIBRARY_PATH=$PWD/lib" -E LD_PRELOAD=/lab/sdl-preview.so \
    ./DynaBrothers1AOT --no-launcher --widescreen off --window-size 320x240 "$@" roms/db1.md
echo PREVIEW_BENCH_OK
$B poweroff -f
"""


def deb_payload(file):
    package = file.read_bytes()
    pos = 8
    while pos < len(package):
        size = int(package[pos + 48:pos + 58])
        if package[pos:pos + 16].startswith(b"data.tar"):
            return io.BytesIO(package[pos + 60:pos + 60 + size])
        pos += 60 + size + size % 2
    raise ValueError(f"No data archive in {file}")


def add_modules(out, userspace):
    contents = deb_payload(HERE / ("kernel-modules-amd64.deb" if userspace else "kernel-modules.deb"))
    wanted = {"virtio", "virtio_ring", "virtio_mmio", "virtio_blk", "virtio_console",
              "virtio_pci", "virtio_pci_modern_dev", "virtio_pci_legacy_dev",
              "btrfs", "xor", "xor-neon", "raid6_pq", "xxhash", "blake2b_generic", "zstd_compress",
              "zstd_decompress", "zstd_common", "crc32c_generic", "libcrc32c"}
    parents = set()
    with tarfile.open(fileobj=contents) as archive:
        for member in archive:
            name = Path(member.name)
            if "modules" not in name.parts or not member.isfile():
                continue
            if not (name.name.split(".ko")[0] in wanted or name.name.startswith("modules.")):
                continue
            path = Path("usr/lib") / Path(*name.parts[name.parts.index("modules"):])
            for parent in reversed(path.parents):
                if parent.as_posix() not in parents and parent not in (Path("."), Path("usr"), Path("usr/lib")):
                    parents.add(parent.as_posix())
                    cpio(out, parent.as_posix(), stat.S_IFDIR | 0o755)
            cpio(out, path.as_posix(), stat.S_IFREG | 0o644, archive.extractfile(member).read())


def make_initrd(path, interactive, probe, systemd, stress, input_script, play, preview_bench,
                userspace, bench_frames, bench_input, virtual_pad, candidate, paced, archive, full_preview):
    installer = HERE / ("installer-amd64-initrd.gz" if userspace else "installer-initrd.gz")
    with installer.open("rb") as source, path.open("wb") as dest:
        shutil.copyfileobj(source, dest)
    with gzip.open(path, "ab", compresslevel=3) as out:
        add_modules(out, userspace)
        cpio(out, "darkos-probe", stat.S_IFREG | 0o755, USERSPACE_INIT if userspace else INIT)
        cpio(out, "lab", stat.S_IFDIR | 0o755)
        cpio(out, "lab/check.sh", stat.S_IFREG | 0o755, CHECK)
        if probe:
            cpio(out, "vm-probe-only", stat.S_IFREG | 0o644)
        if interactive:
            cpio(out, "lab/interactive", stat.S_IFREG | 0o644)
        if play:
            clang = shutil.which("clang") or r"C:/Program Files/LLVM/bin/clang.exe"
            names = ("sdl-preview", "sdl-preview-original") if preview_bench and not userspace else ("sdl-preview",)
            for name in names:
                library = HERE / f"{name}.so"
                subprocess.run([clang, "--target=aarch64-linux-gnu", "-fuse-ld=lld", "-nostdlib",
                                "-fPIC", "-shared", "-O3", "-Wall", "-Wextra", "-Werror",
                                SOURCE / f"{name}.c", "-o", library], check=True)
                cpio(out, f"lab/{name}.so", stat.S_IFREG | 0o644, library.read_bytes())
        if userspace:
            with tarfile.open(fileobj=deb_payload(HERE / "qemu-user-amd64.deb")) as package:
                binary = next(member for member in package if member.isfile() and member.name.endswith("/qemu-aarch64"))
                cpio(out, "lab/qemu-aarch64", stat.S_IFREG | 0o755, package.extractfile(binary).read())
            if virtual_pad:
                cpio(out, "lab/virtual-pad", stat.S_IFREG | 0o644)
            if full_preview:
                cpio(out, "lab/preview-60", stat.S_IFREG | 0o644)
            if preview_bench:
                cpio(out, "lab/preview-bench", stat.S_IFREG | 0o644, f"{bench_frames}\n".encode())
                if paced:
                    cpio(out, "lab/bench-paced", stat.S_IFREG | 0o644)
                if bench_input:
                    cpio(out, "lab/bench.input", stat.S_IFREG | 0o644, bench_input.read_bytes())
        if stress:
            cpio(out, "lab/stress.sh", stat.S_IFREG | 0o755, (SOURCE / "stress.sh").read_bytes().replace(b"\r\n", b"\n"))
            cpio(out, "lab/stress-settings", stat.S_IFREG | 0o644, stress)
            fault = HERE / "sdl-fault.so"
            clang = shutil.which("clang") or r"C:/Program Files/LLVM/bin/clang.exe"
            subprocess.run([clang, "--target=aarch64-linux-gnu", "-fuse-ld=lld", "-nostdlib",
                            "-fPIC", "-shared", "-Wall", "-Wextra", "-Werror", SOURCE / "sdl-fault.c",
                            "-o", fault], check=True)
            cpio(out, "lab/sdl-fault.so", stat.S_IFREG | 0o644, fault.read_bytes())
            if input_script:
                cpio(out, "lab/scenario.input", stat.S_IFREG | 0o644, input_script.read_bytes())
        if systemd:
            cpio(out, "lab/units", stat.S_IFDIR | 0o755)
            cpio(out, "lab/units/r36s-lab.target", stat.S_IFREG | 0o644, b"""
[Unit]
Description=R36S lab
DefaultDependencies=no
Requires=r36s-lab.service
After=r36s-lab.service
""".lstrip())
            cpio(out, "lab/units/r36s-lab.service", stat.S_IFREG | 0o644, b"""
[Unit]
Description=Run the port inside dArkOS
DefaultDependencies=no
[Service]
Type=oneshot
ExecStart=/run/r36s-lab/check.sh
StandardOutput=tty
StandardError=inherit
TTYPath=/dev/console
""".lstrip())
        if archive and not probe:
            parents = {"lab"}
            with zipfile.ZipFile(archive) as port:
                for file in port.infolist():
                    if file.is_dir():
                        continue
                    name = Path("lab") / file.filename
                    for parent in reversed(name.parents):
                        if parent.as_posix() not in parents and parent != Path("."):
                            parents.add(parent.as_posix())
                            cpio(out, parent.as_posix(), stat.S_IFDIR | 0o755)
                    permissions = (file.external_attr >> 16) & 0o777
                    replacement = candidate and file.filename == "dynabrocollection/DynaBrothers1AOT"
                    contents = candidate.read_bytes() if replacement else port.read(file)
                    cpio(out, name.as_posix(), stat.S_IFREG | permissions, contents)
            from tools.build import DB1_SHA, DB2_SHA, default_input_root, find_by_hash
            for index, digest in enumerate((DB1_SHA, DB2_SHA), 1):
                try:
                    rom = find_by_hash(default_input_root(), digest, {".md", ".bin"})
                except RuntimeError:
                    continue
                cpio(out, f"lab/dynabrocollection/roms/db{index}.md", stat.S_IFREG | 0o600, rom.read_bytes())
        cpio(out, "TRAILER!!!", 0)


def report_stress(result, requested):
    cases = {}
    pattern = r"SCENARIO_BEGIN (\w+)\n(.*?)SCENARIO_END \1"
    for name, log in re.findall(pattern, result, re.S):
        series = re.findall(r"GENESISRECOMP_BENCHMARK (\{[^\n]+\})", log)
        run = json.loads(series[-1]) if series else None
        keys = ("SCENARIO_EXIT", "nr_throttled", "throttled_usec", "memory.peak", "oom", "oom_kill")
        record = {key: int(re.search(rf"^{re.escape(key)} (\d+)", log, re.M).group(1)) for key in keys}
        record["runtime"] = run
        issues = [line for line in log.splitlines() if line.startswith(("SDL_", "Killed", "[SDL-FAULT]"))]
        record["diagnostics"] = issues
        timing = re.findall(r"\[HOST-TIMING\] events=(\d+) retained=(\d+) max_frame_us=(\d+)", log)
        if timing:
            record["hitch_events"], record["retained_hitches"], record["max_frame_us"] = map(int, timing[-1])
        presentation = re.findall(r"\[SDL-PRESENT\] samples=(\d+) p50_us=(\d+) p95_us=(\d+) p99_us=(\d+) max_us=(\d+)", log)
        if presentation:
            fields = ("samples", "p50_us", "p95_us", "p99_us", "max_us")
            record["presentation"] = dict(zip(fields, map(int, presentation[-1])))
        cases[name] = record
    if set(cases) != {"software", "cpu_limited", "memory_limited", "missing_drm", "missing_renderer",
                      "slow_present", "upload_failure"}:
        raise SystemExit("The guest did not complete all stress scenarios")
    for name in ("software", "cpu_limited"):
        record = cases[name]
        if record["SCENARIO_EXIT"] or not record["runtime"] or not record["runtime"]["complete"]:
            raise SystemExit(f"{name} did not complete")
        if record.get("presentation", {}).get("samples", 0) < requested["frames"] // 2:
            raise SystemExit(f"{name} did not record enough presentation intervals")
    if requested["cpu_budget"] < 90 and not cases["cpu_limited"]["nr_throttled"]:
        raise SystemExit("CPU throttling was not observed")
    if not cases["missing_drm"]["SCENARIO_EXIT"] or not cases["missing_drm"]["diagnostics"]:
        raise SystemExit("SDL did not report the missing DRM device")
    for name in ("slow_present", "upload_failure"):
        if not any(line.startswith("[SDL-FAULT]") for line in cases[name]["diagnostics"]):
            raise SystemExit(f"The SDL fault was not injected in {name}")
    baseline, throttled = (cases[name]["runtime"] for name in ("software", "cpu_limited"))
    comparison = {"slowdown_ratio": throttled["seconds"] / baseline["seconds"],
                  "state_matches": throttled["state_fnv1a64"] == baseline["state_fnv1a64"],
                  "audio_state_matches": throttled["audio_state_fnv1a64"] == baseline["audio_state_fnv1a64"]}
    report = {"scope": "QEMU virtual hardware, not measured R36S performance", "requested": requested, "scenarios": cases}
    report["comparison"] = comparison
    path = HERE / f"darkos-stress-{requested['game']}.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("Stress report:", path)


def interactive_console(cmd):
    with socket.socket() as port_reservation:
        port_reservation.bind(("127.0.0.1", 0))
        port = port_reservation.getsockname()[1]
    cmd[cmd.index("-serial") + 1] = f"tcp:127.0.0.1:{port},server=on,wait=on,nodelay=on"
    vm = subprocess.Popen(cmd)
    try:
        for _ in range(100):
            if vm.poll() is not None:
                raise RuntimeError("QEMU exited before opening the console")
            try:
                conn = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except ConnectionRefusedError:
                time.sleep(0.1)
        else:
            raise RuntimeError("QEMU console did not become ready")
        with conn:
            conn.settimeout(None)

            def forward_input():
                for line in sys.stdin:
                    try:
                        conn.sendall(line.encode())
                    except OSError:
                        return

            threading.Thread(target=forward_input, daemon=True).start()
            try:
                while data := conn.recv(32768):
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
            except ConnectionResetError:
                # QEMU on Windows can reset the serial connection at VM exit.
                pass
        if vm.wait(timeout=8):
            raise RuntimeError("QEMU returned an error")
    finally:
        if vm.poll() is None:
            vm.terminate()
            vm.wait(timeout=8)


def latest_port_archive():
    pattern = re.compile(r"DynaBrothers-v(\d+)\.(\d+)\.(\d+)-r36s-arm64-alpha(\d+)\.zip")
    releases = []
    for path in (ROOT / "releases").glob("*-r36s-arm64-alpha*.zip"):
        if match := pattern.fullmatch(path.name):
            releases.append((tuple(map(int, match.groups())), path))
    return max(releases)[1] if releases else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--interactive", action="store_true")
    parser.add_argument("--play", action="store_true", help="Play the R36S ARM64 build with a Windows preview")
    parser.add_argument("--preview-bench", action="store_true")
    parser.add_argument("--bench-frames", type=int, default=240)
    parser.add_argument("--bench-input", type=Path)
    parser.add_argument("--paced", action="store_true", help="Measure using the game's normal pacer")
    parser.add_argument("--candidate", type=Path, help="Use this DB1 ARM64 ELF inside the isolated benchmark")
    parser.add_argument("--archive", type=Path, help="Run the exact PortMaster ZIP under test")
    parser.add_argument("--qemu-dir", type=Path, help="Directory containing the Windows QEMU executables")
    parser.add_argument("--userspace", action="store_true", help="Experimental ARM64 process translation on an accelerated Linux guest")
    parser.add_argument("--virtual-pad", action="store_true", help="Route keyboard and XInput through an SDL virtual gamepad")
    parser.add_argument("--preview-fps", type=int, choices=(30, 60), default=30,
                        help="Send every frame or every other frame to the Windows preview")
    parser.add_argument("--control-dir", type=Path, help="New local folder for named pad commands and screenshots")
    parser.add_argument("--cores", type=int, choices=range(1, 5))
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--systemd", action="store_true")
    parser.add_argument("--stress", action="store_true")
    parser.add_argument("--game", choices=["db1", "db2"], default="db1")
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--ram", type=int, default=1024, help="Guest RAM in MiB")
    parser.add_argument("--cpu-budget", type=int, default=35, help="Percent of one vCPU in the throttled scenario")
    parser.add_argument("--memory-limit", type=int, default=12, help="Port cgroup limit in MiB in the memory scenario")
    parser.add_argument("--input-script", type=Path, help="Replay the same input script in all stress scenarios")
    opts = parser.parse_args()
    if not (384 <= opts.ram <= 4096 and 10 <= opts.cpu_budget <= 100 and
            8 <= opts.memory_limit <= opts.ram and 30 <= opts.frames <= 1200):
        parser.error("Use 384–4096 MiB guest RAM, 10–100% CPU, at least 8 MiB port RAM and 30–1200 frames")
    if opts.stress and (opts.interactive or opts.systemd or opts.probe):
        parser.error("--stress runs unattended and independently from other modes")
    use_preview = opts.play or opts.preview_bench
    if opts.userspace and not use_preview:
        parser.error("--userspace applies to --play and --preview-bench")
    if opts.virtual_pad and not opts.userspace:
        parser.error("--virtual-pad requires --userspace")
    if opts.preview_fps == 60 and not opts.userspace:
        parser.error("--preview-fps 60 requires --userspace")
    if opts.control_dir and not (opts.play and opts.virtual_pad):
        parser.error("--control-dir requires --play --virtual-pad")
    if use_preview and (opts.interactive or opts.systemd or opts.probe or opts.stress or (opts.play and opts.preview_bench)):
        parser.error("--play opens its own interactive game window")
    if opts.input_script and (not opts.stress or not opts.input_script.is_file()):
        parser.error("--input-script requires stress mode and an existing file")
    if (opts.bench_input or opts.bench_frames != 240) and not (opts.userspace and opts.preview_bench):
        parser.error("--bench-input and --bench-frames require --preview-bench --userspace")
    if (opts.candidate or opts.paced) and not (opts.userspace and opts.preview_bench):
        parser.error("--candidate and --paced require --preview-bench --userspace")
    if opts.candidate and not opts.candidate.is_file():
        parser.error("--candidate must exist")
    if opts.archive and not opts.archive.is_file():
        parser.error("--archive must exist")
    if opts.bench_input and not opts.bench_input.is_file():
        parser.error("--bench-input must exist")
    if opts.systemd and (opts.probe or opts.interactive):
        parser.error("--systemd starts the laboratory target unattended")
    host_arch = "x86_64" if opts.userspace else "aarch64"
    qemu_name = f"qemu-system-{host_arch}.exe"
    qemu_dir = opts.qemu_dir or os.environ.get("DYNA_R36S_QEMU_DIR")
    fallback = ROOT.parent / "ReKHCoM" / "toolchain" / "qemu" / "runtime" / qemu_name
    qemu = Path(qemu_dir) / qemu_name if qemu_dir else Path(shutil.which(qemu_name) or fallback)
    if not qemu.is_file():
        parser.error(f"Cannot find {qemu_name}; provide --qemu-dir")
    initrd = HERE / ("darkos-pad-check-initrd.gz" if opts.virtual_pad else
                    ("darkos-userspace-workload-initrd.gz" if opts.preview_bench else "darkos-userspace-initrd.gz") if opts.userspace else
                    "darkos-preview-benchmark-initrd.gz" if opts.preview_bench else "darkos-play-initrd.gz" if opts.play else
                    f"darkos-stress-{opts.game}-initrd.gz" if opts.stress else "installer-probe.gz" if opts.probe else
                    "darkos-systemd-initrd.gz" if opts.systemd else "darkos-lab-initrd.gz")
    if opts.control_dir:
        opts.control_dir = opts.control_dir.resolve()
        opts.control_dir.mkdir(parents=True, exist_ok=False)
        initrd = opts.control_dir / "boot-initrd.gz"
    stress = f"{opts.frames} {opts.cpu_budget * 1000} {opts.memory_limit} {opts.game}\n".encode() if opts.stress else None
    make_initrd(initrd, opts.interactive, opts.probe, opts.systemd, stress, opts.input_script,
                use_preview, opts.preview_bench, opts.userspace, opts.bench_frames, opts.bench_input, opts.virtual_pad,
                opts.candidate, opts.paced,
                opts.archive or latest_port_archive(),
                opts.preview_fps == 60)
    cmd = [
        qemu, "-machine", "q35,pic=off" if opts.userspace else "virt",
        "-cpu", "max" if opts.userspace else "cortex-a35", "-smp", str(opts.cores or (2 if use_preview else 4)), "-m", str(opts.ram),
        "-display", "none", "-serial", "stdio", "-monitor", "none", "-nic", "none", "-no-reboot",
        "-kernel", HERE / ("linux-amd64" if opts.userspace else "linux-arm64"), "-initrd", initrd,
        "-drive", f"file={opts.image.resolve()},format=raw,if=none,id=os,readonly=on",
        "-device", "virtio-blk-pci,drive=os" if opts.userspace else "virtio-blk-device,drive=os",
        "-append", ("console=ttyS0" if opts.userspace else "console=ttyAMA0") + " rootfstype=ramfs rdinit=/darkos-probe quiet panic=1",
    ]
    if use_preview:
        cmd.extend(["-accel", "whpx" if opts.userspace else "tcg,thread=multi,tb-size=256"])
    if opts.systemd:
        cmd[-1] += " systemd.unit=r36s-lab.target systemd.log_target=console"
    if opts.interactive:
        interactive_console(cmd)
        return
    if opts.play:
        from preview import play_window
        play_window(cmd, HERE, opts.userspace, opts.virtual_pad, opts.control_dir, opts.preview_fps)
        return
    if opts.preview_bench:
        from preview import benchmark_preview
        benchmark_preview(cmd, HERE, opts.userspace, opts.bench_frames, opts.bench_input, opts.virtual_pad,
                          "darkos-port-candidate" if opts.candidate else "darkos-port-baseline" if opts.paced else None)
        return
    log = HERE / (f"darkos-stress-{opts.game}.log" if opts.stress else "installer-probe.log" if opts.probe else
                  "darkos-systemd.log" if opts.systemd else "darkos-lab.log")
    with log.open("wb") as out:
        subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT,
                       timeout=90 + int(opts.frames * (0.4 + 6 / opts.cpu_budget)) if opts.stress else 150, check=True)
    result = log.read_text(errors="replace")
    print(result)
    if opts.stress:
        if "DARKOS_STRESS_DONE" not in result:
            raise SystemExit("The dArkOS stress suite did not complete.")
        report_stress(result, {key: value.name if isinstance(value, Path) else value for key, value in vars(opts).items()})
        return
    if not opts.probe and "DARKOS_USERSPACE_OK" not in result:
        raise SystemExit("The dArkOS userspace check did not complete.")
    if opts.systemd and "PID 1: systemd" not in result:
        raise SystemExit("The dArkOS systemd target did not boot.")


if __name__ == "__main__":
    main()
