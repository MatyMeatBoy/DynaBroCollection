# Dyna Brothers 1 + 2 — native port work

Development milestone, **not a finished port**. Current scope and evidence are in [STATUS.md](STATUS.md).

The supplied Japanese dumps remain untouched in `D:/01Claude/Dynabrothers`. ROMs, extracted assets, generated game code, binaries and private test captures stay outside Git.

## What currently exists

- Hash-qualified BPS/IPS tooling and a verified English v0.1 copy of Dyna Brothers 1.
- A new **partial** Dyna Brothers 2 Special English BPS patch: 22 gameplay prompts. The supplied retail IPS must not be applied to Special.
- Windows x64 development runners that execute statically emitted 68000 C operations with current-byte validation. No 68000 interpreter fallback is enabled in these AOT targets. The Z80 sound CPU still uses the engine's interpreter.
- Original Japanese and patched English DB1 share one AOT executable. DB2 accepts its original Japanese image and the exact scoped English image.
- Six-button pad protocol by default; explicit saved three-button settings are retained. Physical controller acceptance remains pending.

The usable development targets are `DynaBrothers1AOT` and `DynaBrothers2AOT`. Legacy `DynaBrothers1EN` / `DynaBrothers2JP` targets are failed function-based experiments, retained for diagnosis; they are not qualified game runners.

## Reproduce locally

Dependencies: Python 3, CMake, Ninja and a C/C++ toolchain with SDL2. This machine uses MSYS2 UCRT64/GCC 16.2. The build helper finds that installation without changing the machine-wide PATH.

```powershell
python tools/build.py --inputs 'D:\01Claude\Dynabrothers' --target both
python -m unittest discover -s tests -v
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

Use the original Japanese ROM path for Japanese. Language switching currently means starting the same executable with the other qualified image; the requested launcher/in-game toggle has not been implemented. Do not treat this development entry point as the final launcher.

Native probes use separate `GENESIS_USER_DIR` profiles, remove diagnostic interpreter overrides, require completed finite workloads, and check errors beyond the process exit status. `--audio` records uncapped PCM through SDL's dummy output; it does not certify audible device behavior, tempo, or latency. Uncapped audio queue overflow is recorded separately from CPU execution acceptance.

## Sources and credits

- [GenesisRecomp](https://github.com/mstan/segagenesisrecomp), pinned at `4b2247d2b33a45623a686043a48c2021744c39ab`, with [the local explicit-PC/IRQ patch](patches/0001-explicit-pc-mainloop.patch). Its PolyForm Noncommercial terms and third-party licenses remain applicable.
- Shared `m68k-recomp-core` AOT instruction ABI, pinned through the engine's submodule. The runtime retains its exception and byte-validation contracts.
- DB1 English patch supplied locally; its CRC agrees with [Tybis's published v0.1 identity](https://retroachievements.org/forums/topic/15658).
- DB2 English prompt vocabulary adapted from the supplied IPS; its author/version has not been independently established. [The catalog](translations/db2special-en.json) records guarded Special addresses and provenance.
- [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX) is used only as an independent local reference, never linked into the native port. Its downloaded DLL hash is recorded by each reference probe.
- [OptiPort](https://github.com/MatyMeatBoy/OptiPort), commit `9673006504bf6712b320da70965bc3f65f2dd41a`, guides preservation and validation.

Recent framework/game repositories and exact revisions are recorded in [upstream-pins.json](docs/research/upstream-pins.json). Availability of another project's feature does not establish that it works in Dyna Brothers.

DB1 map-label revision: `python tools/translate_db1.py local/roms/db1-en-v01.md` creates `local/roms/db1-en-v02.md` and a BPS add-on for English v0.1. Six map/surrender labels are now readable and checked in both runtimes; this is still a partial project. The build helper also produces a CRC-named copy and accepts it in the AOT runner.
