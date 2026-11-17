# Work board — 2026-11-17

**The requested project is unfinished.** No stable release, full translation, finished launcher, or widescreen mode is being claimed.

## Completed work with bounded evidence

| Work | Evidence and limit |
|---|---|
| Protected dump/patch inventory | SHA256/CRC32 identities in initial audit; original MD checksums correct |
| DB1 English patch application | Output CRC32 `C1E5C0DA`; independent reference reaches English intro, options and first gameplay |
| Retail DB2 IPS incompatibility | Writes strings over Special instructions; changes 379,723 of its 524,288 extension bytes. Not applied as a supported patch |
| Special English adaptation | 22 guarded, fixed-capacity gameplay prompts; no ROM resize; MD checksum recomputed; source/target/patch CRCs and BPS roundtrip checked |
| Special English visible control | Independent reference and native tutorial capture show `PRESS START`; the other 21 prompts await scene-specific visual checks |
| Native 68000 execution | Explicit-PC AOT operations rather than the initial incorrect function-based route. DB1 reaches first gameplay in EN and JP; DB2 reaches early tutorial gameplay in the scoped EN image |
| DB2 compact calls | Original F-line exception handler executes through the AOT MC68000 exception ABI; not replaced with guessed game behavior |
| DB2 RAM program | Original compressed code at ROM `$E852` is recovered to `$FFE294`, 3,398 bytes; emitted ahead of time and guarded against current RAM bytes |
| DB2 RAM return stub | `$FF0070` RTS byte/stack/cycle contract tested, including rejection of another opcode |
| Correct regional hardware | Japanese NTSC version register rather than bypassing the game's regional check |
| H-int contract repair | Real frame/mask/44 entry cycles. Forced H-int probe fails before and passes after; both V-int and H-int checks observed |
| Source pin integrity | Build helper checks exact approved tracked patch plus recursive submodule pins/cleanliness; negative tests reject additional tracked/untracked edits |
| Resource codecs | Bounded LZ and dictionary formats recovered from original routines; synthetic tests and 33 observed dictionary assets decoded. Not all game resource formats are qualified |

## Current local artifacts

- `local/roms/db1-en-v01.md`: supplied DB1 translation.
- `local/roms/db1-en-v02.md`: six additional map/surrender label repairs; CRC32 `3BE23E09`, SHA256 `04d8be46c5e0597efa3315537a694e8cc314e66c0e519420cab9ba222195a4c4`.
- `local/patches/db1-en-v02-addon.bps`: 206-byte add-on, applies to supplied English v0.1 output (not directly to Japanese).
- `local/roms/db2special-en-v02.md`: **partial** Special translation; CRC32 `DAC9BAFC`, SHA256 `157c75e691528da42abf87e1074d26dca96c14151026be1bd4f8d288889be2db`.
- `local/patches/db2special-en-v02.bps`: 461-byte delta; SHA256 `4e1b3e46611ccd3f37d111f79ee8a16d91f7d876ff50cc4ec350f9b5aeb50971`.
- `build/dev/DynaBrothers1AOT.exe`, `build/dev/DynaBrothers2AOT.exe`: mutable development builds. Not packaged or published.

These local files are not Git inputs. Reproduce from source/catalogs and the user's qualified dumps. Previous partial patch versions and all originals are retained.

## Verification ledger

- 23 Python tests: patch corruption/source/bounds, BPS writer roundtrips, codec limits, translation guards/layout/checksum, and engine-source pin rejection.
- CTest `db2_ram_instruction_contract`: valid guarded RTS and wrong-byte rejection.
- DB1 native EN and JP: 6,010-frame menu-to-gameplay probes; captures under `local/evidence`.
- DB2 native scoped EN: 10,010-frame early tutorial probe and screenshots.
- Audio PCM captured with uncapped dummy output. Large queue drops in those accelerated runs are **not** accepted as real-time sound behavior; physical audio output and sound fidelity remain unverified.
- H-int before/after: `local/evidence/hint-before-forced` fails; `hint-after-forced` passes and observes IRQ levels 4 and 6.
- A separate read-only reviewer identified H-int and source-pin issues; both were fixed and the reviewer found no remaining substantive issue in those two fixes.
- Exact executable hashes and measured frame counts belong to each `probe.json`; changes after a probe require new evidence. Finite execution does not prove campaign or timing equivalence.

## Outstanding authorized work, in order

1. Finish a verified text/graphics inventory for DB1, including practice, results, endings and secrets; complete genuinely missing English rather than guessing from patch version numbers.
2. Complete Special's English: unit names, menus, tutorials, story, scenarios, result/ending text and graphical text. Candidate extraction found 684 byte sequences, including false positives: **not** a translation denominator. Do not bulk-patch those candidates.
3. Visually exercise all 22 scoped prompts and their layout/meaning. Establish ROM-backed regression captures for each context.
4. Resolve remaining timing/color differences against independent reference, audio acceptance, lifecycle, saves and longer routes. Whole-game correctness remains unverified.
5. Implement translation toggle enabled by default in the final shared launcher/in-game settings. Current language selection by ROM path is only a development mechanism.
6. Implement the requested launcher, in-game menu, Original/Enhanced modes and persistent shared settings. Verify actual 3-/6-button hardware and remapping.
7. Original must preserve the requested 4:3 presentation; distinguish CRT pixel-aspect correction from raw square-pixel integer scaling. Current engine labels alone are insufficient.
8. Implement real 16:9 world extension, terrain and actors outside the MD frame, UI/cursor alignment, and largest integer scaling at 1080p/1440p/4K. No widescreen capability is implemented or exposed yet.
9. Qualify host state snapshots before enabling saving/rewind in the new AOT path; current development adapters explicitly disable them. Original DB2 SRAM remains separate from snapshot acceptance.
10. Package and verify a standalone build with notices and no proprietary inputs. No remote repository, PR, release or upload was created.

## DB1 map-label milestone

Recovered the compact font-selection table at `$1B510` and six nine-tile labels at `$1B464`. The supplied patch changed the font but left these Japanese indices, producing garbled text. Repaired CREATURES, LAND MAP, EGG MAP, SURRENDER, NO and REALLY? with guarded data edits; no code or ROM-size change. All six visually checked in the independent reference and current native runner (`local/evidence/db1-v02-all-map-native`, 8,900 frames). The route previously missed AOT PC `$AF26`; expanded the aligned candidate catalog over the observed DB1 program region `$200..$30000` rather than enabling an interpreter fallback. Counts include data candidates. The existing English defeat/results screen was also visually checked in the reference.

The native probe now relocates screenshot basenames and fails on missing/invalid captures. `current-db2-en` has finite-execution evidence only because its nested captures failed; `current-db2-en-captures` repeats the route with both valid captures. This does not alter earlier probe results retroactively.

## Reversible language-system milestone

Added a default-on English selector backed by qualified sparse ROM data (DB1: original, supplied v0.1, current v0.2; DB2: original Special/current partial English). ROM files on disk are untouched; all spans and the full image CRC are checked before mutation, and invalid target metadata restores the source. Generated cartridge payloads stay private under `generated/language`.

Pinned shared `recomp-ui` at `d55bf8304c455dbc5773a000c919f6ba0a26d6bb`, MIT. The graphical launcher was visually opened: six-button default and checked English feature observed; unchecking it, pressing Play, persisted `english=0` and booted Japanese text. Both games passed finite native routes from original Japanese inputs with default English and from translated inputs with Japanese selected (DB1 8,900; DB2 10,010 frames; `local/evidence/language-*`). These exact executable hashes precede the additional region-label/runtime-item seam.

Added game-owned runtime-menu item/get/set seams to the approved engine patch, a Japanese region caption, and an English (restart) in-session item with transactional preference persistence. The in-session preference callback is covered by CTest, but visual opening of the runtime menu is still unverified: the Windows automation keyboard attempts produced no SDL key events in `runtime-menu-input/run.log`; no game-input fix is claimed from that. Mouse interaction with the launcher and its actual Japanese boot were observed.

Three CTests pass: language patch transaction/rollback, language preference/provider persistence, and RAM RTS contract. Shared preboot UI is integrated, not a finished combined two-game launcher or Original/Enhanced mode system.

Additional authorized controls work: automatic 8BitDo M30 Bluetooth and 2.4GHz profiles with six-button Genesis layout, SDL2/XInput/DirectInput and generic joystick support. Research found distinct M30 IDs in DirectInput; some XInput/adapter identities are generic, so those connections require an explicit M30 profile override. No physical M30 validation has occurred.

## v0.1.1 checkpoint consolidation

The user requested a local checkpoint, a rounded-card launcher, an OptiPort optimization pass and feedback upstream. The combined Windows hub now launches either shared game UI and waits for the child to close before enabling another launch. Package preparation remains a candidate until exact packaged regressions and visual checks pass.

- Controller support now registers configurable mappings for previously unmapped SDL joysticks and retains SDL's XInput/DirectInput backends. M30 physical-button presets and per-device overrides are exposed in launcher and runtime settings. Device keys combine GUID and a hash of serial/path/name; raw identities are not published. Shared/generic IDs retain the explicit override path.
- Actual SDL virtual-device tests cover eight M30 buttons, explicit three-button filtering, generic fallback, disconnect/reconnect, P1/P2 isolation, XInput trigger/face mapping and per-device persistence. Physical transports/firmware remain unverified.
- A reviewer found replay restoration wrongly required connected controllers and persisted recorded physical-layout preferences. Failure reproduced in the production path (`replay-controller-before`); replay callbacks now ignore valid host-layout restores while recorded logical input masks remain authoritative. Production record/playback then matched all 300 executions (`replay-controller-after`). Reviewer verified the fix.
- A 1,024-slot native descriptor cache retains ordinary live opcode/extension validation on every hit. Synthetic tests verify extension changes on hot hits, distinct modules at one PC and unchanged state on misses.
- OptiPort comparison, three alternating runs per policy, one binary, logical core 2 on AMD Ryzen 7 7435HS: median CPU time 14.140625 s binary search vs 12.9375 s cached for 10,010 DB2 frames (8.508% lower). Ranges overlap; state/audio hashes equal. This is uncapped CPU evidence, not physical display/audio latency. The later replay-only and launcher changes do not alter the lookup implementation; packaged correctness is checked separately.
- OptiPort refreshed to `58843a2c9122720f5db8649c0d9cfdb60dc479cf`, installed Markdown synchronized with backups. The preserved local launcher complement is still absent upstream. Feedback published with the matching local MatyMeatBoy GitHub account: https://github.com/MatyMeatBoy/OptiPort/issues/28 . No proprietary input or private log was uploaded.
- Current source checks: 26 Python tests and five CTests. The final checkpoint's binary/package hashes and packaged results belong to its closure manifest, not these mutable development paths. Package staging rebuilds clean sources and queries the copied executables' embedded revision/version with only packaged/Windows DLL paths; stale generated headers are not proof of compiled identity.

Full translations, actual expanded widescreen, final Original/Enhanced presentation, physical-device acceptance and AOT save/rewind qualification remain open after this checkpoint.
