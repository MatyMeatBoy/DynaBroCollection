# Pending work queue (clean list, audited 2026-11-18 ~17:05)

Rules: take one task, branch `claude/<topic>`, PR to `work/dynabrothers`, then `bash tools/integrate_prs.sh` (merges open PRs after the Python suite passes). Do not edit `HANDOFF-2026-11-18.md`; keep notes in your own `docs/handoff/<topic>.md`. Never publish ROMs, dumps or `local/` evidence.

Setup (Git Bash): put `D:/01Claude/DynaBrothers/toolchain/ucrt64/bin`, `git/cmd`, `python` first in PATH; `export DYNA_TOOLCHAIN_BIN=D:/01Claude/DynaBrothers/toolchain/ucrt64/bin`. Checkout `C:\DynaWork\DynaBrothers`. After any translation change: `python tools/generate_db2_glyph_text.py` (if glyph_text/ram_text changed), `python -m tools.translate_db2 <rom> --output local/roms/X.md --patch local/patches/X.bps` (second `crc32` = new English CRC, put it in `games/hub_settings.c` and `games/launcher_discovery.c` replacing the previous one), `python -m tools.build --target both --jobs 8 && cmake --build build/dev --parallel 8`, then `ctest.exe` in `build/dev` and `python -m unittest discover -s tests -p "test_*.py"`. If `tools.build` fails in `check-engine-adapter` after a merge that changed `patches/0001-*.patch`: `git -C upstream/segagenesisrecomp apply --reverse <old patch from git show <commit>^:patches/...>` and rerun (never reset --hard/clean there).

Probe: `python -m tools.native_probe build/dev/DynaBrothers2AOT.exe "<DB2 rom>" --script <input> --output <NEW dir> --frames N --language en` (output dir must not exist). Debug: `DYNA_TRACE_GLYPH=1|2`, `DYNA_DB2_NO_HOST_TEXT=1`, `tools/glyph_trace_report.py`.

## DONE and verified on screen (do not redo)
Stage List/Versus menus, password selector, Versus settings and COM names, 17 Versus stage names, title OPTIONS menu and sound-test names, story intro + lab dialogue (RAM script, 19 lines), battle dialogue, all six biographies, Story stage 1 and Practice 1 briefing bodies, gameplay prompt boxes (3-line fix, RAIN verified), tutorial/prompt catalog. Video: Normal2 16:9 battle fallback fixed; overlays leave guest/audio hashes identical. DB1: Extras yellow-selection fixed by another agent (`33ac139`), keyboard A/B/C drive the Extras menu (`step_adapter.c keyboard_menu_bits`, needs manual keyboard test).

## DONE but NOT verified on screen
- Story briefings stages 2-41 and Practice 2-6 (entries `briefing-*`, 0x220FF9-0x2225A9). Stage list STORY only offers stage 01 (progress-locked); needs a password/flag that unlocks later stages, or playing through.
- VS/Dyna Fight labels (CONTROLS/HIT/DEF/SMASH/ROLL/SELECT/1P-2P WINS/DRAW/character names, 0x164162-0x164332), 2P setup (`2P SETUP`, `6 BUTTON PAD`), Sega Channel special stage names (0x20C784...), `glyph16` names for stages not shown yet.

## OPEN (not started or blocked)
1. Event dialogue inside Story stages (RAM script stream): needs gameplay to trigger; add `ram_text` entries (`DYNA_TRACE_GLYPH=1`, `[glyph-ram]` lines, 6-word unique prefix, see `games/db2_glyph_text.c`).
2. Graphical text (tile images, need graphics patch or host overlay like `games/db2_bio_overlay.c`): "れんしゅう N" briefing title, main-menu rows (れんしゅう/ストーリー/オリジナル/たいけつ モード), anything else drawn as tiles.
3. Original-mode stage list line `01:ORIGIN 始まりの小島` (kanji name): not drawn through the two known glyph renderers (0x5B0FE hooked, 0x5B22E traced only); find its renderer.
4. Capture-verify: QUIT/rematch dialogs (`*s`), result screens, Dyna Fight screens, Sega Channel, endings. Reach requires Extras corner/passwords (`DBSAMA`→`UHYO`→`MAKOTU` unlock route in `evidence/db2-quit-proof`).
5. Owner build screen "YOU WENT EXTINCT!!" (HITS/KILLS/ALIENS/TIME/PASSWORD, CONTINUE/SAVE GAME) is not in this checkout's ROM text; compare against the owner's build to see what remains.
6. Native-speaker review of stage/character/briefing wording (translations are best effort and abbreviated to fit byte capacity).
7. `controller_profiles_contract` aborts on this machine (SDL virtual joysticks; a physical USB pad and `SDL_GAMECONTROLLERCONFIG` exist here; unsetting them did not help). Try with no pad connected/another PC; else debug `gamepad_init`/`s_pad[0]`.
8. Physical controller recognition and audio on the reporter PC (cannot be tested here).
9. Packaging after merges: `python -m tools.package_checkpoint stage local/candidate-next` (clean tree; remove old `local/candidate-next` first).
10. Expected, no action: native fallback frames before battle and during stage-intro transitions (reasons 01/04/10/20); producer/SAT coherence research remains optional (`evidence/db2-visor-research/coherence.md`).
