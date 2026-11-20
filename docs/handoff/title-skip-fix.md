# DB2 title/main menu after skipping the intro (fixed in v0.5.1)

## Symptom (reported on an AMD RX 6900 XT PC, 2560 × 1440, 100 Hz)

The DB2 main menu shows:
- the original Japanese mode plates (れんしゅう/ストーリー/オリジナル/たいけつ モード) instead of PRACTICE/STORY/ORIGINAL/VERSUS;
- a strip of unrelated tiles along the top and bottom edges;
- a purple "4 ル" fragment of the title logo in the left margin.

Another PC (NVIDIA) showed the menu correctly, so it looked GPU-specific.

## Cause

It is not the GPU, the resolution or the portable folder. It depends on **how the title is reached**:
- Pressing START during the intro (as you do when entering with a controller, e.g. the M30) makes the guest lay out the title and mode menu **on different name-table rows** and compensate with the vertical scroll.
- In that layout the sky row on plane B moves from row 11 to row 3, and the mode plates on plane A move from rows 3/6/9/12 to rows 7/10/13/16.

Two detections in the DB2 compositor were tied to the rows of the normal (unskipped) sequence:
1. `title_sky_planes()` (`games/db2_video.c`) looked for the sky row only at row 11. When it failed, the title was not recognised, so:
   - the parked title-logo sprites were not hidden (the "4 ル");
   - the sky was not anchored/continued, so plane B rows 28–31 (leftover intro tiles 0x756–0x770) showed in the top and bottom bands.
2. `db2_menu_labels_frame()` (`games/db2_menu_labels.c`) assigned PRACTICE/STORY/ORIGINAL/VERSUS by fixed rows 3/6/9/12, so no plate matched and no English label was drawn.

Waiting for the intro, or a probe that never skips it, takes the row-11 layout. That is why every scripted check passed and the NVIDIA PC looked right.

## Fix

- `title_sky_planes()` accepts the solid sky signature on any of the 32 plane-B rows (same three cells and same sky tile pattern as before).
- Each mode plate is identified by its own first Japanese letter tile instead of its row:
  - 199 → PRACTICE (176 when the first cell has not streamed in yet)
  - 1DB → STORY
  - 1E3 → ORIGINAL
  - 1C3 → VERSUS

## Verification

- Same binary and ROM, 2560 × 1440, English and 16:9.
- Main menu reached by waiting for the intro and by skipping it with START at frames 40, 120, 400, 900, 1500 and 1900. Every capture shows the four English labels in order, no logo fragment and no edge strips.
- Reproduction scripts: `local/amd/skip.input`, `local/amd/sk<frame>.input`.
- `db2_video_contract` passes (its streamed-plate case uses tile 176).

## How to reproduce a "only on my PC" report next time

1. Read `data/logs/*-feedback.replay.config.ini` and `.mods.txt` from the user's install for the exact settings.
2. Run the user's own `data/DynaBrothers2AOT.exe` with `tools.native_probe --portable-env` and the same window size.
3. If the probe looks right, compare the route: in particular whether the intro was skipped.

## Follow-up queue (2026-11-20, after v0.5.1)

Done on branch `claude/menu-perf`:
- **Original / Versus menus stuttered.** `remap()` in `games/db2_wide_menu.c` walked the dense run for every margin pixel. It is now memoised per frame (`remap_cached`), and frame time in the Original menu dropped from 11.8 to 3.7 ms.

Open:
1. **DB2 units shown twice or "blending" while panning with B + D-pad** (repro: Original → SEGA CHANNEL SPECIAL → stage 1, hold B + LEFT/RIGHT; route `local/amd/sc2.input`, needs `--frames 5000`).
   - Cause: during a pan a unit's native SAT body is sometimes not recognised (`kind=0`). Since v0.3.1 such entries are placed from their native pixels (to avoid a 4:3 flash), so the unit is drawn twice: once from RAM by the host, once from the SAT copy, about one sub-step apart.
   - Tried and rejected:
     - dropping unknown entries near any host actor: the egg vanished for two frames;
     - shifting host actors by ±`off_x/off_y`: the queen still doubled in one frame.
   - Next step: identify the unit's own SAT body by its pattern range and animation frame (`db2_actor_native_sat_body`), and drop only that copy when the host draws the unit.
   - Check the start of the stage too (similar report).
2. Check DB1 for the same pattern (`db1_video.c` places non-actor native sprites; actor bodies are filtered by `actor_owner`).
3. Start+Select menu: move the "System" section to the very end. The game list already ends with it, but the engine appends its own sections after the game items (`runner/main.c` ~3464 builds `cfg.extra_items`), so this needs a change in the ordering there or in recomp-ui.
4. DB1: remove the six-button option (DB1 does not support it) and add manual mapping.
5. Launcher:
   - "Skip launcher on boot" off by default;
   - borderless by default;
   - logo intro with fade, then a large selector that fits the screen;
   - VSync always on, plus an option for it and more graphics options.
6. Launcher settings: there are two configuration screens today (one opens from the Settings button, another appears after Back from it). Merge everything into one configuration screen and open it directly from Settings (launcher and in-game "Full Settings").
