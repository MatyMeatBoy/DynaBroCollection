# DynaBroCollection

[![Release](https://img.shields.io/badge/release-v0.5.4%20Beta-brightgreen)](https://github.com/MatyMeatBoy/DynaBroCollection/releases/tag/v0.5.4-beta)
[![Source checks](https://github.com/MatyMeatBoy/DynaBroCollection/actions/workflows/source-checks.yml/badge.svg)](https://github.com/MatyMeatBoy/DynaBroCollection/actions/workflows/source-checks.yml)

<a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Get it on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="48"></picture></a>

![Dyna Brothers 1 + 2](docs/images/dyna-logo-1plus2-v1.png)

DynaBroCollection is a native recompilation of *Dyna Brothers* and *Dyna Brothers 2 Special* for Sega Genesis / Mega Drive. Both games use your own compatible Japanese cartridge dumps and start in English with a fresh profile.

## Download and play

Download [v0.5.4 Beta](https://github.com/MatyMeatBoy/DynaBroCollection/releases/tag/v0.5.4-beta) for your platform and bring your own compatible cartridge dumps.

- **Windows x64** — extract the ZIP and run `DynaBrothers.exe`. Place dumps in `roms/` or select them in the launcher. Keep the adjacent `data/` folder with the executable.
- **Linux x64** — extract the archive and run `DynaBroCollection`. Requires glibc 2.36 or later. The native settings picker asks for the ROM the first time.
- **macOS Apple Silicon** — extract the archive and open `DynaBroCollection.app`. The beta is an ad hoc build. A Mac may require you to allow the app in **System Settings → Privacy & Security** after the first open attempt.
- **Android 7.0 or later** — install the APK, then choose a ROM for each game from the collection screen. The APK contains arm64-v8a and x86_64 executables and on-screen controls. The 0.5.4 beta installs alongside the previous Android app and uses a separate save directory.

To play in Japanese, disable **English translation** in **Settings → Mods → Language** and restart the game. The source dumps stay untouched. Change the fullscreen option in **Settings** to switch to windowed or exclusive fullscreen. The selected display mode is saved to the profile.

## Features

- **Dyna Brothers** has a full-screen Extras menu and a scrolling stage selector. Completing a difficulty opens its starting stage and the starting stages of lower difficulties. The widescreen lightning effect reaches both sides of the playfield.
- **Dyna Brothers 2 Special** keeps its widescreen framing during gameplay and pause. Enhanced mode defaults to a floating HUD. The original HUD remains available.
- **DB2 presentation** includes a continuous title sky, intact clouds at the right edge, flat green Versus panels, and original proportions for menus and Game Over.
- **English DB2 text** covers the verified Practice, Sega Channel Special, stage selection, briefing, and results routes. English coverage across both games is still partial.
- **Launcher and settings** include a centered, scalable cover, cartridge animation, and tabbed settings available in-game through **Start + Select**, including **Return to Title** and **Quit Game**.
- **Achievements** include local progress, badge notifications and an in-game list. Both games can also send live unlocks to Recomp Achievements. [Achievement sets, eligibility and setup](docs/recomp-achievements.md)

See the [v0.5.4 release notes](https://github.com/MatyMeatBoy/DynaBroCollection/releases/tag/v0.5.4-beta) for the release scope and the [status board](STATUS.md) for project history.

## Build and verify

Use Python 3, CMake, Ninja, and an SDL2-capable Windows x64 toolchain such as MSYS2 UCRT64. Put the exact supported dumps in the private `Dynabrothers/` input folder. The build helper checks the ROM hashes, pinned engine sources, and adapter patch before generating the native targets.

```powershell
python tools/build.py --target both
cmake --build build/dev
ctest --test-dir build/dev --output-on-failure
python -m unittest discover -s tests -v
```

The game targets are `DynaBrothers1AOT` and `DynaBrothers2AOT`. They execute statically emitted 68000 operations with runtime byte validation. The engine still interprets the Z80 sound CPU. Native probe output and private test inputs belong under `local/`.

The release workflow uses `tools.package_checkpoint stage` from a clean source commit, `tools.verify_checkpoint` to test the staged executables without development DLL paths, and `tools.package_checkpoint close` to produce the ZIP and SHA-256 file.

## Frontend integration

RecompLauncher reads [recomplauncher.json](recomplauncher.json) from the release folder. Each tile takes its own Japanese ROM dump and starts the selected game. Automatic window mode follows the launcher's desktop or Big Picture mode. Explicit choices in the shared settings file take precedence. Windows uses the portable `data/` profile when writable, with a fallback under `%LOCALAPPDATA%\DynaBroCollection`.

Use the root `DynaBrothers.exe` as the portable entry point. The two AOT executables in `data/` are child processes. `data/DynaBrothersLauncher.exe --integration-info` prints the launcher arguments as JSON. See [recomp-launcher.json](recomp-launcher.json) for supported ROM, game, and display arguments.

## Project links and credits

- [Changelog](CHANGELOG.md) and [contributing guide](CONTRIBUTING.md)
- [Translation catalog](translations/db2special-en.json) and [local achievement contracts](docs/db1-achievement-contracts.md)
- [GenesisRecomp](https://github.com/mstan/segagenesisrecomp), [recomp-ui](https://github.com/mstan/recomp-ui), and [SDL GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)
- [Third-party notices and licenses](THIRD_PARTY_NOTICES.md) and [upstream revisions](docs/research/upstream-pins.json)
