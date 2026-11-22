# GoodLuckOS on GA36-MB

This target uses ARMv7 hard float (`/lib/ld-linux-armhf.so.3`) on Allwinner A23/A33. The Rockchip RK3326 AArch64 package needs a separate build. GoodLuckOS 0.1.0 provides SDL2 and SDL3 with KMSDRM and ALSA support.

## Package paths

- `HOME/.local/share/applications/dynabrocollection.puppy`
- `HOME/ports/dynabrocollection/launch.sh`
- `HOME/ports/dynabrocollection/DynaBrothers1AOT` and `DynaBrothers2AOT`
- `HOME/ports/dynabrocollection/roms/db1.md` and `db2.md` for the player's own dumps

Puppy reads the `.puppy` file and launches each game through `/bin/sh`. The launcher reads the firmware controller mapping and writes `db1-last-run.log` or `db2-last-run.log` under `user`. Assets and licenses belong next to the executables. These launcher files require an ARMv7 build before installation.

## Debug targets

Use the firmware root filesystem and a Cortex-A7 QEMU userspace run to check ARM instructions, dynamic linking and scripted game logic. Use dummy SDL backends for deterministic frame and state comparisons. Measure frame pacing with the Mali display path, ALSA audio and the physical controller on the handheld.

On a Linux host with `qemu-arm` installed, run `sh platform/goodluckos/smoke.sh FIRMWARE_ROOT ARMV7_STAGE` to check both engines against the firmware loader. `FIRMWARE_ROOT` is the extracted Linux partition, with its symlinks preserved. The smoke check prints the dependency resolution and build metadata for each executable.

- [GoodLuckOS firmware releases](https://github.com/CodeZombie/goodluckOS/releases)
- [Puppy configuration](https://github.com/CodeZombie/goodluckOS/blob/main/rootfs/package/puppy/PUPPY.md)
- [GA36 Ports installation guide](https://ga36.genserek.net/help/)
