# Dyna Brothers 1 + 2 — checkpoint v0.1.1

[![Checkpoint 0.1.1](https://img.shields.io/badge/version-0.1.1%20checkpoint-orange)](docs/releases/v0.1.1-checkpoint.md)
[![Source checks](https://github.com/MatyMeatBoy/DynaBrothers1-2/actions/workflows/source-checks.yml/badge.svg)](https://github.com/MatyMeatBoy/DynaBrothers1-2/actions/workflows/source-checks.yml)
[![Target Windows x64](https://img.shields.io/badge/target-Windows%20x64-0078D4)](docs/releases/v0.1.1-checkpoint.md)
[![Translations partial](https://img.shields.io/badge/translations-partial-orange)](STATUS.md)
[![Runtime license](https://img.shields.io/badge/runtime-PolyForm%20Noncommercial-5c6b7a)](THIRD_PARTY_NOTICES.md)

Development milestone, **not a finished port**. Current scope and evidence are in [STATUS.md](STATUS.md).

Development after the closed checkpoint adds guarded English credits names and the hard-ending secret-mode message for DB1. See [the recovered layout, output identity and verification limits](docs/research/db1-credits-translation.md). These changes do not replace the existing v0.1.1 ZIP.

Start `DynaBrothersLauncher.exe` to choose a game from the shared card-based hub. See [v0.1.1 checkpoint notes](docs/releases/v0.1.1-checkpoint.md) for the exact included scope and pending work. Complete translations, true expanded 16:9, Original/Enhanced modes and final 4:3 pixel-aspect treatment remain unfinished.

![Launcher v0.1.1 with two rounded game cards](docs/images/launcher-v0.1.1.jpg)

Local launcher capture; [image/executable provenance](docs/images/launcher-v0.1.1.json). Packaged launch is verified separately. The source-check badge covers public tooling fixtures, not cartridge gameplay or physical controller acceptance.

| Resource | Purpose |
| --- | --- |
| [Checkpoint notes](docs/releases/v0.1.1-checkpoint.md) | Install, controls, included work and limitations |
| [Closed checkpoint verification](docs/releases/v0.1.1-verification.md) | Compiled revision, archive identity and bounded regression results |
| [Changelog](CHANGELOG.md) | Changes in v0.1.1 |
| [Work board](STATUS.md) | Evidence and outstanding work |
| [Contributing](CONTRIBUTING.md) | Reproduction and review expectations |
| [Third-party notices](THIRD_PARTY_NOTICES.md) | Runtime, UI, input and font attribution |

The supplied Japanese dumps remain untouched in `D:/01Claude/Dynabrothers`. ROMs, extracted assets, generated game code, binaries and private test captures stay outside Git.

## What currently exists

- Hash-qualified BPS/IPS tooling, the supplied English DB1 v0.1 and six additional repaired map/surrender labels.
- A new **partial** Dyna Brothers 2 Special English BPS patch: 22 gameplay prompts. The supplied retail IPS must not be applied to Special.
- Windows x64 development runners that execute statically emitted 68000 C operations with current-byte validation. No 68000 interpreter fallback is enabled in these AOT targets. The Z80 sound CPU still uses the engine's interpreter.
- Original Japanese and patched English DB1 share one AOT executable. DB2 accepts its original Japanese image and the exact scoped English image.
- Six-button pad protocol by default; explicit saved three-button settings are retained. Physical controller acceptance remains pending.
- Shared graphical launcher, default-on reversible language selection, per-device M30 presets and generic SDL joystick support. Virtual-controller hotplug/isolation is tested; physical Bluetooth/2.4GHz/DirectInput hardware qualification remains pending.

The usable development targets are `DynaBrothers1AOT` and `DynaBrothers2AOT`. Legacy `DynaBrothers1EN` / `DynaBrothers2JP` targets are failed function-based experiments, retained for diagnosis; they are not qualified game runners.

## Reproduce locally

Dependencies: Python 3, CMake, Ninja and a C/C++ toolchain with SDL2. This machine uses MSYS2 UCRT64/GCC 16.2. The build helper finds that installation without changing the machine-wide PATH.

```powershell
python tools/build.py --inputs 'D:\01Claude\Dynabrothers' --target both
python -m unittest discover -s tests -v
cmake --build build/dev --target DynaLanguagePatchTest DynaStepLookupTest DynaControllerTest DynaLanguageSettingsTest DynaRamContractTest
ctest --test-dir build/dev --output-on-failure
```

The helper checks exact input hashes, the pinned engine commit, its approved patch, and recursive submodules. It refuses additional engine changes rather than overwriting them. Build logs are under `local/evidence/build`.

To prepare the current scoped Special patch independently:

```powershell
python tools/translate_db2.py 'D:\01Claude\Dynabrothers\Dyna Brothers dumped\Dyna Brothers 2 Special (Japan) (Virtual Console).md' --output local/roms/db2special-en-v02.md --patch local/patches/db2special-en-v02.bps
```

That command creates new files exclusively. Existing outputs must be preserved under another name, or the command must use a new destination.

## Development execution

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
build/dev/DynaBrothers1AOT.exe 'D:\01Claude\Dynabrothers1+2\local\roms\db1-en-v01.md' --pacing accurate --no-launcher
build/dev/DynaBrothers2AOT.exe 'D:\01Claude\Dynabrothers1+2\local\roms\db2special-en-v02.md' --pacing accurate --no-launcher
```

English is enabled by default with either qualified input language. In **Mods > Language**, disable **English translation** for Japanese on the next start. The original files on disk remain untouched. The restart-required in-session item is connected, but its visual interaction still needs verification.

**Mods > Controls** offers Automatic, Custom SDL bindings and M30 DirectInput/Bluetooth, Mini 2.4GHz and XInput profiles. Shared or hidden hardware identities require the explicit preset. A profile is saved for the connected device without replacing another controller's bindings.

Native probes use separate `GENESIS_USER_DIR` profiles, remove diagnostic interpreter overrides, require completed finite workloads, and check errors beyond the process exit status. `--audio` records uncapped PCM through SDL's dummy output; it does not certify audible device behavior, tempo, or latency. Uncapped audio queue overflow is recorded separately from CPU execution acceptance.

## Sources and credits

- [GenesisRecomp](https://github.com/mstan/segagenesisrecomp), pinned at `4b2247d2b33a45623a686043a48c2021744c39ab`, with [the local explicit-PC/IRQ patch](patches/0001-explicit-pc-mainloop.patch). Its PolyForm Noncommercial terms and third-party licenses remain applicable.
- Shared `m68k-recomp-core` AOT instruction ABI, pinned through the engine's submodule. The runtime retains its exception and byte-validation contracts.
- DB1 English patch supplied locally; its CRC agrees with [Tybis's published v0.1 identity](https://retroachievements.org/forums/topic/15658).
- DB2 English prompt vocabulary adapted from the supplied IPS; its author/version has not been independently established. [The catalog](translations/db2special-en.json) records guarded Special addresses and provenance.
- [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX) is used only as an independent local reference, never linked into the native port. Its downloaded DLL hash is recorded by each reference probe.
- [OptiPort](https://github.com/MatyMeatBoy/OptiPort), current pass at `58843a2c9122720f5db8649c0d9cfdb60dc479cf`, guides preservation, measured optimization and package validation. [Feedback contributed](https://github.com/MatyMeatBoy/OptiPort/issues/28).
- [recomp-ui](https://github.com/mstan/recomp-ui), MIT, provides the shared graphical settings UI. [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) supplies pinned device mappings.

Recent framework/game repositories and exact revisions are recorded in [upstream-pins.json](docs/research/upstream-pins.json). Availability of another project's feature does not establish that it works in Dyna Brothers.

DB1 map-label revision: `python tools/translate_db1.py local/roms/db1-en-v01.md` creates `local/roms/db1-en-v02.md` and a BPS add-on for English v0.1. Six map/surrender labels are now readable and checked in both runtimes; this is still a partial project. The build helper also produces a CRC-named copy and accepts it in the AOT runner.

The native targets now use the shared graphical launcher on a plain double-click (passing a ROM path directly bypasses it). English is enabled by default even when loading the original Japanese dump. In Mods > Language, disable **English translation** to restore Japanese on the next start. The selection is shared by the two executables in `dynabrothers-language.ini` beside the selected profile settings. Translations remain partial. An in-session restart-required item is wired, but its visual interaction still needs verification.

`tools/package_checkpoint.py stage` collects native binaries, transitive DLLs, assets and notices from clean source. `tools/verify_checkpoint.py` runs the packaged regressions with development DLL paths excluded. `package_checkpoint.py close` creates an immutable ZIP, portable folder and hashes; it refuses to overwrite a closed delivery.
