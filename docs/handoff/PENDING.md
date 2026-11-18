# Pending work queue (keep this file current; one task per bullet)

Rules for agents: take one task, branch `claude/<topic>`, open a PR to `work/dynabrothers`, run `bash tools/integrate_prs.sh` to merge open PRs after the Python suite passes. Do not edit `HANDOFF-2026-11-18.md` (merge hotspot); put notes in your own `docs/handoff/<topic>.md`. Never publish ROMs, dumps or `local/` evidence.

Dev setup (Git Bash): put `D:/01Claude/DynaBrothers/toolchain/ucrt64/bin`, `git/cmd`, `python` first in PATH; `export DYNA_TOOLCHAIN_BIN=D:/01Claude/DynaBrothers/toolchain/ucrt64/bin`. Checkout: `C:\DynaWork\DynaBrothers`. Full rebuild after any translation change: `python -m tools.build --target both --jobs 8 && cmake --build build/dev --parallel 8`, then `ctest` in `build/dev` and `python -m unittest discover -s tests -p "test_*.py"`. The English CRC changes with every ROM patch: update `games/hub_settings.c` and `games/launcher_discovery.c` (get it from `python -m tools.translate_db2 <rom> --output local/roms/X.md --patch local/patches/X.bps`, second `crc32`).

## Translation (DB2 Special)
1. Find and translate more 16x16 glyph strings: run an input script with `DYNA_TRACE_GLYPH=1`, then `python -m tools.glyph_trace_report run.log <rom> out.png`; every `TODO` becomes a `glyph_text` entry (see `docs/handoff/HANDOFF-2026-11-18.md`, "Overlay de cadenas de glifos"). Scenes not yet traced: story stages after the intro, Original mode play, Sega Channel stages, endings, practice results, password screen.
   - Checked: the title-screen OPTIONS menu (おんがく/こうかおん/ぼたんはいち/A・B・C labels, main menu row 6) produces **zero** `[glyph]` hits at PC `0x5B174` when traced (confirmed twice, two separate input scripts/navigation paths). It is not driven by either known glyph mechanism (glyph16 or glyph_text) — it needs a new renderer PC to be found (disassemble around the options-menu draw call) before it can be translated; `glyph_trace_report` alone won't find it.
2. Kanji stage names of Original mode (`01:ORIGIN 始まりの小島` etc.): not seen by `glyph_trace_report`, find how they are drawn. `パスワード入力` is done (ENTER PASSWORD).
3. Graphical stage briefings ("STAGE n" text): tile text, not ROM glyph strings; needs a host overlay like `games/db2_bio_overlay.c`.
4. Capture-verify unverified strings: STEGO biography, VS/Dyna Fight screens (character select, controls, results), 2P setup (`6 BUTTON PAD`), special stage names, QUIT/rematch dialogs (`*s`), result screens.
5. Have a Japanese speaker review stage/character names (readings in `translations/db2special-en.json` are best effort).

## Recomp / video
6. Reasons 01/04/20 fallbacks before battle and reason 10 during stage-intro transitions are expected; remaining coherence work is producer/SAT latch research (`evidence/db2-visor-research/coherence.md`).
7. Compare guest/audio state hashes with overlays on vs off (bio overlay, glyph text) over the 9400-frame fixture.
8. Physical controller recognition and audio on the reporter PC (cannot be tested here).

## Packaging
9. After merges: `python -m tools.package_checkpoint stage local/candidate-next` (needs clean tree and `DYNA_TOOLCHAIN_BIN`).

## Notes from tracing (renderer coverage)
- Glyph renderers: 0x5B0FE (ROM strings, hooked at 0x5B174) and 0x5B22E (words >= 0x1E00 use a caller-supplied glyph table, trace tag `[glyph2]` at 0x5B278, not hooked). The Original-mode stage list line `01:ORIGIN 始まりの小島` is NOT drawn through either in the traced route; find its renderer (probably another PC) before translating it. `DYNA_TRACE_GLYPH=1` prints both tags.
## Reported by the owner (2026-11-18, from the internal test build)
10. DB2 "YOU WENT EXTINCT!!" results/game-over screen (HITS/KILLS/ALIENS/TIME/PASSWORD, CONTINUE/SAVE GAME menu): owner says it still needs more translation work ("falta extender esta pantalla aun") — labels shown are already English but something on this screen is incomplete; needs its own capture pass to find what's left (compare against the full screen, not just the menu box).
11. (ANALYSIS: the Extras row is drawn by `games/db1_title_menu_art.c` line ~112 with palette bank 1 (`|0x2000`) whenever `selected==4`; there is no 'chosen' state. Next: capture the native palette bank used by chosen items right after START on rows 0-3 and use it for row 4 once `modal` is set; `Db1TitleMenu.modal` is available from `db1_title_menu_frame`.) UI bug (likely DB1 title/hub menu, not DB2): selecting "Extras"/Password stays highlighted yellow instead of turning white like other selections when chosen. Needs repro capture + the native code path for that menu's selection-color state.
12. (FIXED in `games/step_adapter.c` `keyboard_menu_bits`, needs manual keyboard verification; the engine maps only arrows/Return/Space/Esc to host-menu actions, so the keys bound to pad A/C/B now also feed `GP_OVERLAY_A/B` in `menu_tick`) Input bug, same Extras menu: keyboard A/B/C keys don't navigate/control it (owner: "controles de teclado de a b c no dejan manejar el menu ese de extras"). Check the keyboard-to-pad binding path for whatever control scheme that menu expects.
