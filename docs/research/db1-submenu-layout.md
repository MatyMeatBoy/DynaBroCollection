# DB1 authored submenu layout

Date: 2026-11-18. Original guest controller flow; DB2 paused and untouched.

## Native evidence and the missing extension

Copied `build/dev/DynaBrothers1AOT.exe` to
`local/submenu-agent/db1-baseline.exe` before probing. SHA256:
`38bfc2432141feb97a35dbff770c0cb8576d3dc7d3b2bbf2ca9cd86f9ca5f5f2`.
Prepared private ROM SHA256:
`ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261`.
No ROM writes, guest state writes, state loading, PC jumps or source changes
were used for captures. Each native probe used an independent profile.

`local/submenu-agent/maps-wide/creatures.png` visibly shows the complete
320px native map UI centered inside a 398px screenshot with black side margins.
This is real missing presentation coverage. In contrast, the gameplay right HUD
already has plane A relocation in `db1_video.c`; its native x208..319 span moves
to the wide right edge. The map scene disables world composition altogether.

The old `games/db1/scenes/map-labels.input` is not adequate visual coverage:
its six screenshots were byte-identical in the baseline run. It sends the
cycling buttons while the map initializer is still working. The finite probe
passing does not mean those menu states were reached.

`games/db1/scenes/submenu-native.input` waits 3288 frames after the START press,
then uses normal A/C presses. The original-mode native captures under
`local/submenu-agent/native-qualified/` were visually inspected and show:
CREATURES, LAND MAP, EGG MAP, SURRENDER, NO and REALLY?. This fixture intentionally
uses accurate screenshot names for the actual states. It does not qualify an
orders or hazards page merely by naming a screenshot after one.

`games/db1/scenes/options-native.input` uses two DOWN presses at the title;
`local/submenu-agent/options-original/submenu-options.png` shows OPTION.
The existing `progress-native.input` with a longer final wait was independently
run in original mode, producing the PASSWORD grid in
`local/submenu-agent/password-original/progress-password-native.png`.

## Ownership from actual ROM code and VRAM

| Guest entry | Ownership |
| --- | --- |
| `19980` | Map/command screen initialization; clears scroll state, calls `1ACD8`, decodes `6FC00`, uploads map/UI patterns, builds map/legend. Existing provider calls this `map_overlay`. |
| `19D06` | Native map menu controller. Logical command 3 cycles map modes; 1/2 control confirmation state; 4 exits. Physical assignment comes from RAM `FF0028..2B`. |
| `1B040` | Existing map ownership exit hook. |
| `1E024` | Options ownership, already classified by `db1_scenes`. |
| `1D7B6` | Password ownership, already classified by `db1_scenes`. |

Read-only Capstone disassembly of the preserved prepared ROM code and native
`DUMP_VRAM`/`DUMP_RAM` output supplied the following bounds. Native dump commands
only observe state.

* Map plane A is at C000, 64 tiles per row. The main frame is x16..208,
  y16..208; its two-track corners/vertical caps are 16px wide. The horizontal
  stroke uses repeated tile 059C with the authored flips on the lower edge.
  The complete map interior is x32..192, y32..192: 160 unique pixels per row,
  not an infinite terrain strip. The legend/confirmation content occupies
  x216..304, y16..208. Plane B at E000 contains a genuine alternating checker
  from tiles 05A1/05DC, not an inferred extension of the map image.
* Options main frame is x40..280, y56..200. Plane A tiles 20/21/22 with
  palette1 supply corners, sides and repeated horizontal strokes. Most text
  is plane A; selection cursor is SAT tile23 at x66,y72 in the captured state.
  Plane B supplies the purple panel fill using tile24 in columns6..33,
  rows8..23. Thus relocating glyphs alone would split one UI layer.
* Password main frame has the same x40..280 geometry at y72..200. Its whole
  grid, BACK/END and cursor remain one unscaled rectangle. The separate header
  remains native-sized and centered.

## Pure helper and integration contract

`db1_menu_layout_row(scene,line,native,out,width)` runs after the normal scene
background/native-center compositor. It only changes the main options/password
panel rows. The 8px caps remain fixed size at x40 and width-48; authored padding
x48..55 supplies the new interior and horizontal border strokes. The complete
native x48..272 content is copied once at `(width-320)/2+48`. All letter spacing,
values and cursor pixels remain intact together. Original width320 is identity.
The native source row is copied before writes, so aliasing native and output is
supported. Background outside the enlarged panel is untouched.

`db1_menu_layout_map_row(line,native,background,out,width)` is called only while
actual map ownership is active. `background` must be a separately sampled,
authored plane B row (it may alias output). It replaces old flattened center
pixels before adding the UI. The main frame expands to x16..width-112, keeping
its 16px caps. Its complete 160px map is copied once, centered in the enlarged
frame; the uncovered interior shows genuine background padding. The entire
88px legend rectangle moves to width-104..width-16. No unique terrain pixel is
repeated or stretched. Caller owns scene lifetime and background sampling.

ARGB composition preserves the flattened native checker under legend transparent
areas as part of that coherent rectangle. A low-level A/sprite provider could
preserve background phase separately, but must move every related glyph, icon
and sprite together. Do not partially relocate only one plane of the UI.

## Verification

The focused C test exhausts every canvas width320..768 (including odd widths),
checks cap geometry, whole-content identity and coherent anchoring, source
immutability, preserved outer background, map padding, frame-only repetition,
unsupported scenes and invalid geometry. Direct GCC C11 compilation with
`-Wall -Wextra -Werror` and the test executable passed.

These helper tests prove bounded layout contracts. The copied baseline native
captures prove actual missing coverage and routes; they do not prove integrated
wide output. The root integration must capture these same native routes after
applying the helper and visually inspect main frame, label/cursor coherence,
map identity and right-edge legend/HUD positioning.

### Integrated native run (first integration)

Copied the integrated binary, DLLs and assets to
`local/submenu-agent/integrated-bin/` before the run. Integrated executable
SHA256: `bb2f127361220d1dd00e1138db805e8f49a05ed6890ceec255edfbfe4237a00c`.
All probes used `--window-size 2560x1440 --widescreen 16:9`, producing real
427x240 canvas captures. Options, password and all six map states completed
finite execution and were visually inspected in `integrated-wide-options`,
`integrated-wide-password` and `integrated-wide-maps` under the private agent
folder.

Exact same-input/frame-count comparison against the original-mode baseline:

| Route | CPU/RAM state FNV1a64, identical | Audio state FNV1a64, identical |
| --- | --- | --- |
| Map, 12400 frames | `55FABF8FB55B7918` | `ABDBA05FD01DFB57` |
| Options, 6500 frames | `3FF19DF7381F1112` | `08C3819CC07B5D34` |
| Password, 6600 frames | `9E4E02EEE63258CE` | `898914CDF8EE1E8D` |

For each of CREATURES, LAND MAP, EGG MAP, SURRENDER, NO and REALLY?, the
160x160 native map crop `(32,32)..(192,192)` is byte-identical to the integrated
wide crop `(85,40)..(245,200)`. The complete 88x192 legend crop
`(216,16)..(304,208)` is byte-identical to `(323,24)..(411,216)`.
Options/password content inside x48..272 is also byte-identical after one
coherent horizontal translation by53 and vertical translation by8, including
cursors, text and dynamic values. Main frames visibly widen and map terrain
is neither repeated nor stretched.

The first integration also exposed two presentation defects, reported to the
provider owner before claiming completion:

* Sampling arbitrary plane B coordinates behind the widened map interior can
  expose creature legend graphics formerly hidden under native UI. These are
  visible in the REALLY? capture's right interior padding. The authenticated
  blank backing is the native plane B x0..15 alternating checker strip; only
  that period should supply newly uncovered map padding.
* Options/password still have black margins outside their 320px authored
  background span, despite expanded main frames. Their actual backdrop repeats
  must be established separately; the panel helper does not invent background
  pixels or fix an incomplete background sampler.

These are background ownership issues, distinct from the verified intact
foreground layout. A subsequent provider correction needs a new copied binary
and native captures before those issues may be marked resolved.

### Background correction contracts

The follow-up provider request authorized a narrow change to
`db1_scenes.c/.h`: options/password background sampling only. Existing branding
live CRAM behavior, title coordinate API, scene hooks and canvas band ownership
were retained.

The original options writer at `1E1DC..1E238` and password writer at
`1D94E..1D9B2` each write 39 glyph columns from a rolling 16-entry sequence,
followed by a 39-column blank row, for 15 row pairs. Because39 modulo16 is7,
the sequence advances7 glyphs per16px of vertical movement. Its complete
mathematical period is128px horizontally and256px vertically. Options sequence
at ROM `1E738` is `1C1C1C051A0F021C0313101509061314`; password sequence at
`1DF6A` is `9A9A9A83988D809A81918E9387849192`. These are authored text
patterns, not UI labels selected by screenshot recognition.

`db1_scenes_menu_background_b` reads the live untouched name-table row0's first
16 entries and row1's blank entry. It follows the verified writer's phase,
then reads current VRAM patterns, flips and packed palette/priority. The existing
plane coordinate helper supplies live horizontal/vertical scroll. Later panel
fills and native unwritten columns39+ and rows30+ are excluded. The generic
plane B sampler remains unchanged for title and other owners.

`tests/db1_menu_background_test.c` compares every uncovered authored native
pixel in312x240 against the generic plane sampler, with deliberately overwritten
panel cells. It checks signed-coordinate128x256 periodicity, negative phase,
live scrolling, reserved-mode refusal and VDP immutability. Direct GCC C11
`-Wall -Wextra -Werror` compilation and execution passed.

For the map provider, independent native VRAM inspection verified E000's first
two columns across **all32 rows**: `[05A1,05DC]` on even rows and
`[05DC,05A1]` on odd rows. Thus the authenticated map backing is a16x16
checker from source x0..15,y0..15. The provider owner was told to repeat only
this rectangle for new blank padding and canvas bands, preserving signed phase;
this prevents legend graphics in other plane B cells from leaking into padding.


### Final native qualification: resolved

The root provider corrected map backing and captured map ownership for added
canvas rows. The next copied executable SHA256 was
`226ef318d1bbb806078ac3d9da89db3aa912bdfcb2ef59fd70e31ee7e378bf2f`.
`local/submenu-agent/final-wide-maps/` contains all six actual states, now with
an accurately named `submenu-surrender.png`. Visual inspection of REALLY?
confirms the padding contains only the genuine checker, without legend leakage.
All six map and legend crops remain byte-identical to the original baseline,
and final CPU/RAM/audio hashes remain identical to the table above. Added top
8px and bottom8px bands match their native checker counterparts16px away,
byte-for-byte across427 pixels; horizontal16px periodicity also passes.

The final options/password correction additionally excludes the native unwritten
last column39 from the flattened native copy. That column had reintroduced an
8px black stripe despite the correct reconstructed backing. Only the first312
native pixels are copied in these two scenes; all actual headers, frames,
glyphs and cursors end before280. A regression assertion verifies the preserved
312px and replacement of the old black column by the authored backdrop.

Final qualified executable SHA256:
`ba608c516b63380c87506259586cb142b6850f6618b59362103f31f74479b675`.
Private executable/DLL/assets copy: `local/submenu-agent/qualified-bin/`.
The final options/password native captures and probes are in
`local/submenu-agent/qualified-options/` and `qualified-password/`. Both ran at
actual2560x1440 SDL windows with427x240 canvases, passed finite execution with
mixer signal, and were visually inspected. They have continuous authored text
backing across the entire canvas, expanded main frames, no side/band black gaps
or old-column stripe, and intact centered text/cursors. Their native224px content
crops and CPU/RAM/audio hashes remain byte-identical to the baseline table.
The final longer password route is now tracked at
`games/db1/scenes/password-native.input`; it is the exact input used by these
password comparisons.

Final source status: authored panel/map helper, three native input fixtures,
periodic options/password background sampler and two focused tests complete.
Existing scene provenance tests, helper width/alias contracts and periodic
backdrop tests pass. `db1_video.c` integration and CMake test registration belong
to the root agent. No pending helper or scene sampler changes remain. DB2 was
not changed; no source ROM writes, guest state writes, PC jumps, commits, pushes
or releases were performed for this work.
