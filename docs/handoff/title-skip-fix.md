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
