# GoodLuckOS on GA36-MB

This target uses ARMv7 hard float (`/lib/ld-linux-armhf.so.3`) on Allwinner A23/A33. The Rockchip RK3326 AArch64 package needs a separate build. GoodLuckOS 0.1.0 provides SDL2 and SDL3 with KMSDRM and ALSA support.

The ClonDiClon test target is GA36-MB V1.2-20260416 with Allwinner A33, a 640×480 JD9366 MIPI-DSI panel and GoodLuckOS 0.1.0 A33 Rhododendron Panel 1.

The handheld launcher uses fullscreen 4:3 with the widescreen renderer disabled. SDL maps the complete 320×224 game image into a 320×240 logical viewport for the 640×480 panel. The renderer scales the texture to the panel, keeping the input image and HUD within view. Diagnostics log the raw image, logical size, output size and viewport when the geometry changes.

## Package paths

- `HOME/.local/share/applications/dynabrocollection.puppy`
- `HOME/ports/dynabrocollection/launch.sh`
- `HOME/ports/dynabrocollection/DynaBrothers1AOT` and `DynaBrothers2AOT`
- `HOME/ports/dynabrocollection/roms/db1.md` and `db2.md` for the player's own dumps

Extract the port ZIP into the root of the existing FAT32 HOME partition and place your cartridge dumps in the indicated `roms` directory. Restart the handheld and open **Ports**. Puppy reads the `.puppy` file and launches each game through `/bin/sh`. The launcher reads the firmware controller mapping and writes `db1-last-run.log` or `db2-last-run.log` under `user`.

## Debug targets

The qualification ZIP adds **Ports → Dyna Brothers - Diagnostico**. When the ZIP supplies the previous DB1 executable, the diagnostic first runs it with normal cadence. It then runs the new build with normal cadence and uncapped. Each leg uses the same 6200-frame Normal 1 input sequence and VSync off. Let the launcher return to Puppy without pressing gameplay buttons. Find the logs, exit statuses and available system details under `ports/dynabrocollection/debug/device/db1-qualification-<timestamp>/`. A fresh profile per leg keeps the regular saves out of the measurement. The launcher records setup errors and exit codes in `ports/dynabrocollection/debug/qualification-launch.log`.

A qualification ZIP can include `DynaBrothers1Qualification` beside the regular game. The diagnostic enables the DB1 poll-tail experiment for that executable. The regular menu keeps launching `DynaBrothers1AOT`.

The optional device diagnostics ZIP adds a **Diagnostics** category in Puppy. Run the VSync and no-VSync entries for the affected game. DB1 uses the included Normal 1 input script and runs for 6200 frames to reach the battle and its state checks. DB2 runs for 3600 frames and accepts manual input. Each writes its timing report to `ports/dynabrocollection/debug/device/db<game>-<mode>/db<game>-last-run.log`. Diagnostic saves and configuration live under `debug/device`, separate from the normal game profile. Remove `.local/share/applications/dynabrocollection-profile.puppy` from HOME to hide the entries after testing.

Use the firmware root filesystem and a Cortex-A7 QEMU userspace run to check ARM instructions, dynamic linking and scripted game logic. Use dummy SDL backends for deterministic frame and state comparisons. Measure frame pacing with the Mali display path, ALSA audio and the physical controller on the handheld.

On a Linux host with `qemu-arm` installed, run `sh platform/goodluckos/smoke.sh FIRMWARE_ROOT ARMV7_STAGE` to check both engines against the firmware loader. `FIRMWARE_ROOT` is the extracted Linux partition, with its symlinks preserved. The smoke check prints the dependency resolution and build metadata for each executable.

For a paired startup comparison, use the shipped ARMHF executable, the same cartridge and `debug/normal1.input`. Run paced for 1230 frames with a clean profile, `GENESIS_NO_VSYNC=1`, `GENESIS_HOST_TIMING_DUMP=1`, `GENESIS_R36S_TRACE_RASTER_FRAME=1200`, and a lab SDL build that supplies dummy audio/video and a software renderer. Keep the firmware mounted read-only. Compare the physical and guest logs with `python tools/r36s_debug.py HARDWARE.log GUEST.log --match-windows --from-frame 180`. The tool reports executable and cartridge fingerprint matches, identical frame ranges and raster breakdowns when both logs captured the same frame. Renderer differences are reported separately. Guest timings characterize the VM, including CPU translation and host scheduling.

On hardware, use the port's shell from a local terminal with DRM access. For example, `GENESIS_HOST_TIMING_DUMP=1 sh launch.sh 1 --measure-runtime 7200` measures DB1 for 7200 frames at the normal pace. Keep the resulting `user/db1-last-run.log` and use `sh debug/collect.sh` to collect Linux runtime information. Repeat the same scene and compare the frame intervals before changing a setting.

The JD9366 driver declares a 30 MHz pixel clock and 1040×518 total timing, which implies about 55.7 Hz for that mode. Check the active mode and compare the default synchronized run to `DYNA_GOODLUCKOS_VSYNC=0 GENESIS_HOST_TIMING_DUMP=1 sh launch.sh 1 --measure-runtime 7200`. Compare average game ticks, worst frame and visible tearing before changing the default. Disabling swap synchronization leaves the existing Mega Drive pacer active. An `--runtime-uncapped` run measures throughput without that pacer.

## Build and package

Define `DYNA_GOODLUCKOS_SYSROOT` as the ARM hard-float Ubuntu 20.04 GCC 10 sysroot. Use the `armv7-focal.cmake` toolchain, `DYNA_R36S=ON`, `DYNA_LAUNCHER_UI=OFF`, and set `SDL2_DIR` to this directory. Set `DYNA_GOODLUCKOS_SDL2_LIBRARY` to the SDL2 shared library extracted from the chosen firmware image, and `DYNA_GOODLUCKOS_SDL2_INCLUDE` to the include directory containing `SDL2/SDL.h`. Build the `DynaBroCollection` target and stage the CMake install. Run `python tools/package_goodluckos.py STAGE OUTPUT.zip` on the staged install.

- [GoodLuckOS firmware releases](https://github.com/CodeZombie/goodluckOS/releases)
- [Puppy configuration](https://github.com/CodeZombie/goodluckOS/blob/main/rootfs/package/puppy/PUPPY.md)
- [GA36 Ports installation guide](https://ga36.genserek.net/help/)
