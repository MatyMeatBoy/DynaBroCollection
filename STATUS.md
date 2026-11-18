## Development update 2026-11-18 (after v0.1.8) — see docs/handoff/PENDING.md

Work resumed at the owner's request. Source checks: 53 CTest (one environment-dependent controller test currently aborts locally, see PENDING.md) and 111+ Python tests. DB2 Special English catalog now has 385 guarded entries plus 90+ host-rendered glyph strings; the current English CRC32 is recorded in `games/hub_settings.c` / `games/launcher_discovery.c`.

- Video: Normal2 16:9 battle no longer falls back to the native image (unowned SAT entries reject a frame only when visible); hidden terrain under opaque UI and blank guest lines are tolerated.
- Translation: Stage List/Versus/password menus, 17 stage names, VS/Dyna Fight labels, story intro and battle dialogue (host glyph overlay), biography overlay, 41 Story and Practice stage briefings (stage 1 verified on screen, the rest not yet).
- Not translated yet: laboratory/story script dialogue after the intro (RAM script stream), Original-mode kanji stage names, title OPTIONS menu, QUIT/results/Versus dialogs unverified. Debug helpers: `DYNA_TRACE_GLYPH=1`, `tools/glyph_trace_report.py`, `DYNA_DB2_NO_HOST_TEXT=1`; PR integration: `tools/integrate_prs.sh`.

## Final v0.1.8 delivery — STOP

The new version is published, its45 packaged checks passed and the public download was verified. The private personal runtime and complete offline continuation toolkit are separate from public assets. [Closed verification](docs/releases/v0.1.8-verification.md) · [Handoff](docs/handoff/CONTINUE-ON-ANOTHER-PC.md). No further feature development is active.

> Closure override — 2026-11-18: the owner requested the next checkpoint and then STOP. v0.1.8 source passed52 native contracts and70 Python tests; packaged qualification is recorded with the release. No background agent work remains authorized. Earlier “active” entries below are historical. Continue from [the portable development handoff](docs/handoff/CONTINUE-ON-ANOTHER-PC.md). DB2 held-pan widescreen fallback and incomplete translation remain explicit pending work.

# Work board — 2026-11-18

## Published v0.1.7 test checkpoint

The new immutable package passed42 packaged checks. Native-title Extras, automatic Password navigation, canonical save migration, speech translation, ending punctuation and EMI progress are integrated. [Exact delivery identity and limits](docs/releases/v0.1.7-verification.md). Source checks:50 CTests and67 Python tests. DB1 baseline is ready for continued testing; full campaign and physical hardware acceptance remain open. DB2 native wide-view, original visor/effect rules and remaining English inventory are now active bounded work. [Current coordination board](docs/handoff/STATUS.md).

## Historical v0.1.6 test checkpoint

That immutable package passed39 packaged checks. It fixes actual home Settings buttons, adds original-size visor positioning and native code-screen saving, and corrects title/story presentation. [Exact delivery identity and limits](docs/releases/v0.1.6-verification.md). Subsequent native title-menu Extras and level navigation are development work outside that package. Current ownership and pending work: [coordination board](docs/handoff/STATUS.md). Finish DB1 before resuming DB2.

## Historical v0.1.4 test checkpoint

The audio, DB1 weather/widescreen, automatic input, same-window settings and Extras checkpoint is published. [Exact archive/source identity and24 packaged checks](docs/releases/v0.1.4-verification.md). DB2 current modules are integrated; further DB2 work is paused by user request. Remaining work is tracked in [the coordination handoff](docs/handoff/README.md). Native code saves/replay and the guide are initial implementations with documented limits, not a complete campaign acceptance claim.

**The overall project remains in development.** v0.1.3 adds experimental DB1 world expansion and local achievements; full translations, DB2 expansion and full campaign/hardware acceptance remain open.

## v0.1.3 implementation

DB1 now has a read-only expanded-world compositor with right-anchored HUD, original-scene fallback, native 4:3 and default enhanced 16:9 integer presentation. It reads all valid visible actor IDs independently of the fixed 36-entry drawing buffer, preserving native plane and UI sprite priorities. Synthetic tests cover all 253 valid IDs, expanded-only actors, duplicate references, sentinels, ordering and immutable inputs. A native 6,100-frame comparison has equal game/audio state and identical original-area/HUD pixels in the sampled scene. Full campaigns and every effect remain unqualified.

DB1's public catalog has 76 independently implemented local goal contracts and 152 official badges. DB2 Special has 93 stage goals, six cumulative completion milestones and 198 original badges. The paused list, FIFO notifications, atomic local profiles and diagnostic/replay exclusion have tests. The integrated SDL list test held CPU/RAM/VDP unchanged across six presentations and closed through a real SDL Back event. No account synchronization or exact official evaluator equivalence is claimed. [DB1 contracts](docs/db1-achievement-contracts.md); [checkpoint scope](docs/releases/v0.1.3-checkpoint.md).

The v0.1.3 r1 package is closed from clean source `a314a74ea03de0bb62008830413df94f9ec2869b`, with 20 packaged checks passing. ZIP SHA256: `0da16abc82098b394d5ea2525eb287be26c5868a7ddc03f74b7f5726255e386d`. The exact extraction also passed its portable debug script. [Public test release and verification](docs/releases/v0.1.3-verification.md). Historical entries below retain the scope and measurements of their own checkpoints.

## Latest development after the closed checkpoint

Active development version is now **0.1.2**, as requested. The native targets and launcher build with that version from the shared `VERSION` file. The first local 0.1.2 checkpoint is closed and its ten packaged regressions passed, with extracted-ZIP probes and hash verification. See [its exact artifact/source identity](docs/releases/v0.1.2-verification.md). Source checks: 37 Python tests and five native contracts passed. Further development continues separately from this immutable artifact.

A later active DB2 unpause route exposed an IRQ/raster deadlock outside the original paused-tutorial test. Development now uses continuous instruction-boundary IRQ entry for full-program AOT. Both 18,000-frame benchmark/audio routes advance the world with matching state/audio hashes; DB1 maps/languages, forced IRQ and replay controls pass. Current tests: 38 Python and six native contracts. See [the root cause and bounded evidence](docs/research/db2-continuous-native-irq.md); corrected r2 packaging remains a separate pending gate.

The corrected **v0.1.2 r2 is now closed**: eleven package cases passed, the full 18,000-execution gameplay replay matched, and DB1 maps/DB2 active gameplay passed again from the extracted ZIP. [Exact r2 identity and limits](docs/releases/v0.1.2-r2-verification.md). The two earlier closed archives remain unchanged. Further translation and presentation development remains open.

The local v0.1.1 checkpoint is closed and verified; see [its exact archive/source identity](docs/releases/v0.1.1-verification.md). Historical entries below retain their original milestone context.

DB1 now has guarded repairs for 18 credits character names and the nine-row hard-ending secret-mode message. Current English CRC32 is `11688440`; JP, supplied v0.1 and previous v0.2 remain accepted by native language switching. Reference captures and a 52,000-frame prepared native ending route show readable credits names and MHRI message. Both games rebuilt; 32 Python tests, five native contracts, both DB1 language/map routes and the DB2 tutorial passed. See [layout, identities, diagnostic limits and remaining inventory](docs/research/db1-credits-translation.md). The closed checkpoint ZIP remains unchanged; this is newer source/development work.

The follow-up repairs practice and alien-mode completion bodies plus their scrolling congratulation backdrops. Current cumulative English CRC32 is now `950A8FC3` (the `11688440` credits milestone is retained privately). Both screens are readable in native and independent reference captures; 34 Python tests pass. Ordinary first-stage victory was already English and was checked separately. No full campaign or full translation completion is inferred.

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
- `local/roms/db2special-en-v02.md`: **partial** Special translation (57 strings as of 2026-11-18; CRC32 `07FEA88F` with the extended table, launcher still accepts earlier `DAC9BAFC`; the SHA256 below predates the extension), SHA256 `157c75e691528da42abf87e1074d26dca96c14151026be1bd4f8d288889be2db`.
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
