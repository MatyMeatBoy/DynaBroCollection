# GoodLuckOS on GA36-MB

This target uses ARMv7 hard float (`/lib/ld-linux-armhf.so.3`) on Allwinner A23/A33. The Rockchip RK3326 AArch64 package needs a separate build. GoodLuckOS 0.1.0 provides SDL2 and SDL3 with KMSDRM and ALSA support.

The ClonDiClon test target is GA36-MB V1.2-20260416 with Allwinner A33, a 640×480 JD9366 MIPI-DSI panel and GoodLuckOS 0.1.0 A33 Rhododendron Panel 1.

## Package paths

- `HOME/.local/share/applications/dynabrocollection.puppy`
- `HOME/ports/dynabrocollection/launch.sh`
- `HOME/ports/dynabrocollection/DynaBrothers1AOT` and `DynaBrothers2AOT`
- `HOME/ports/dynabrocollection/roms/db1.md` and `db2.md` for the player's own dumps

Extract the port ZIP into the root of the existing FAT32 HOME partition and place your cartridge dumps in the indicated `roms` directory. Restart the handheld and open **Ports**. Puppy reads the `.puppy` file and launches each game through `/bin/sh`. The launcher reads the firmware controller mapping and writes `db1-last-run.log` or `db2-last-run.log` under `user`.

## Debug targets

Use the firmware root filesystem and a Cortex-A7 QEMU userspace run to check ARM instructions, dynamic linking and scripted game logic. Use dummy SDL backends for deterministic frame and state comparisons. Measure frame pacing with the Mali display path, ALSA audio and the physical controller on the handheld.

On a Linux host with `qemu-arm` installed, run `sh platform/goodluckos/smoke.sh FIRMWARE_ROOT ARMV7_STAGE` to check both engines against the firmware loader. `FIRMWARE_ROOT` is the extracted Linux partition, with its symlinks preserved. The smoke check prints the dependency resolution and build metadata for each executable.

On hardware, use the port's shell from a local terminal with DRM access. For example, `GENESIS_HOST_TIMING_DUMP=1 sh launch.sh 1 --measure-runtime 7200` measures DB1 for 7200 frames at the normal pace. Keep the resulting `user/db1-last-run.log` and use `sh debug/collect.sh` to collect Linux runtime information. Repeat the same scene and compare the frame intervals before changing a setting.

## Build and package

Define `DYNA_GOODLUCKOS_SYSROOT` as the ARM hard-float Ubuntu 20.04 GCC 10 sysroot. Use the `armv7-focal.cmake` toolchain, `DYNA_R36S=ON`, `DYNA_LAUNCHER_UI=OFF`, and set `SDL2_DIR` to this directory. Set `DYNA_GOODLUCKOS_SDL2_LIBRARY` to the SDL2 shared library extracted from the chosen firmware image, and `DYNA_GOODLUCKOS_SDL2_INCLUDE` to the include directory containing `SDL2/SDL.h`. Build the `DynaBroCollection` target and stage the CMake install. Run `python tools/package_goodluckos.py STAGE OUTPUT.zip` on the staged install.

- [GoodLuckOS firmware releases](https://github.com/CodeZombie/goodluckOS/releases)
- [Puppy configuration](https://github.com/CodeZombie/goodluckOS/blob/main/rootfs/package/puppy/PUPPY.md)
- [GA36 Ports installation guide](https://ga36.genserek.net/help/)
