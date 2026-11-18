# DB1 native visor geometry and command scope

## Original reference and geometry

Original320x224 engine output is retained in `assets/guide/db1-hud.png`, with
provenance in `db1-capture-provenance.json`. Its command interior is
`[16,208)x[16,208)`: six32px world cells each way, or192x192 pixels.
The complete authored frame occupies tile coordinates `[8,216)x[8,216)`,
208x208 pixels. Its actual white horizontal stroke runs x14..209 inclusive,
196 painted pixels; those strokes must not be extended to the HUD.
The HUD begins at native tile x216. Keeping the HUD right anchored puts that
edge at `216 + canvas_width -320`, e.g.323 on a427px canvas.

`db1_visor_layout` balances the208px frame in the space before that HUD.
At427px the zero-offset frame begins at58. Native white strokes are offset6px
inside the frame's left tile and201px inside its right edge; the HUD white
stroke is1px inside HUD tile x216. Actual427px capture first measured white
edges63/258/324, with63px empty left versus65px right. A measured1px baseline
correction for odd canvas widths yields white edges64/259/324, leaving exactly
64px (eight tiles) of empty space on each side. Authored tile gaps58/57 still
differ by only1px. No fractional coordinate or resampling is used. Offsets are signed integer8px tiles relative
to that baseline. The clamp keeps the frame's tile left at least8 and its tile
right no farther than the original authored HUD adjacency. At427 the valid
range is-6..+7; at320 the only valid result is the original frame x8/shift0.

The provider captures one layout per frame. Its world translation is
`S=frame_x-8`: all terrain sampling, actor culling/positions, world markers,
cursors and native screen-space effect sprites use the same inverse/forward
translation. The HUD retains its separate right-edge translation. The original
frame A tiles8..215 remain intact; HUD tiles216..319 are copied right. The world
remains visible outside the frame, and actors are never clipped to its rectangle.
Actor query bounds include the translated left edge and all added top/bottom
rows. This retains the prior frame-coherent camera/scroll reconciliation.

Host API: `db1_video_set_visor_offset_tiles(int)` stores the requested value;
geometry clamps it on the next frame. The setting survives video reset.
Requested values remain distinct from the clamped offset at a particular width.
Root settings integration owns typed key `db1.visor_offset_tiles`.

## Native scope evidence: visuals are not damage rectangles

A private copy of the unmodified original guest execution path was run with
`--widescreen off`, ROM SHA256
`ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261`.
The copied executable SHA256 is
`7713c7ccffac7de51a10e5587f88b860e8c0ff869a1525961c596825a3b64d05`.
Only the already-declared diagnostic power fixture writes999999 BCD power;
menus and casting use ordinary controller input. No PC, actor, weather, terrain
or RNG writes are used.

`local/db1-visor-agent/lightning-native-off/hazard-120.png` visibly has clouds
across all320 native columns, with bolts outside the192px visor and the HUD
hidden. This is native authored plane-A animation, not evidence of320px damage.
Its8500-frame guest/audio states are `9BA34BFE9337062D` /
`816C73A9922AFD08`, matching the previous qualified lightning scene.

Disassembly shows lightning `120F4` calls visual routine132D8 at12110, then
reads player camera origin at12124/12128 (`FF252C+56A8/56A9 = FF7BD4/7BD5`).
The loops at12140/12142 each run5 down to0 and callEDFC: six-by-six target cells.
The new presentation consequently preserves the native320x224 authored visual
span translated by S; it neither fills the full widened canvas with new clouds
nor incorrectly crops the original global visual to192px. Global CRAM flashes
remain global. Added world rows/columns do not invent authored plane-A effects.

Rain `4046 ->11ECC` reads the same camera origin at11F00/11F04, loops six-by-six
at11F1C/11F1E, mutates eligible cells and writes their world coordinates to181A.
`65F2` rejects relative X/Y greater than5; `6372` writes only its six-by-six
native plane-A weather mask. Both world cell identity and192px visual mask are
preserved. The native active-Rain diagnostic at
`local/db1-visor-agent/rain-native-hold/` captures20 consecutive frames. Its
read-only RAM dump has player weather bit2 set, duration15 and nine affected
cells x17..19/y2..4, all inside the current coarse-camera area x14..19/y1..6.
Some eligible-cell visual changes are subtle at native resolution; no claim is
made that all36 cells receive rain or that this is a damage measurement.

## Bounded command inventory

The main dispatch table at23D6 contains signed word offsets, not long pointers.
Its eight entries resolve as follows. This is a native-source inventory of the
bounded player command dispatch; labels are omitted when not independently
qualified.

| Entry | Native destination | Coordinate contract |
| --- | --- | --- |
|0|4046 ->11ECC Rain|Six-by-six camera-origin cells; native192px mask.|
|1|40EA command/order menu|Callbacks include4360: coarse origin plus cursor16A/16B gives world target;11EA4 stores it for subsequent behavior.11DF8 is a persistent faction/order mode, not permission to expand a rectangle.|
|2|406E ->11E32 egg creation|129BC/129E8 reads faction home coordinates at FF7BE4/7BE5 and searches nearby valid placement; not an expanded visor fill.|
|3|43B6 unit-selection menu|2A26 converts original visor cursor plus coarse camera to a world actor; selected actor ID152 is passed to11F86.|
|4|4642 disaster menu|4682 is five long pointers: lightning120F4, distortion1205E, quake11FDA, tsunami12208, meteor12178.|
|5|46CE ->46E0|Recenter camera around original faction home, clamped to native map limits.|
|6|4552 selection/mark family|2A26 finds a world actor from the native cursor;45B4 stores its ID at7BDE.|
|7|460E|Reads marked actor world cell and passes it to original46E0 camera recentering.|

Quake1200A/1200E and12026/12028, distortion12086/1208A and1209E/120A0,
and meteor121A0/121A4 and121B8/121BA likewise use native camera-origin six-by-six
loops. Tsunami12224/12228 passes that origin to EC44/EB9A faction-area queries;
EC44 forms bounds by adding5. Its later1226E/122C6 helpers also loop six-by-six.
Their original full-screen art/scroll/palette behavior is distinguished from
these gameplay bounds; native projectiles are translated, never duplicated.

2A26 explicitly adds cursor16A/16B to camera7BD4/7BD5 before occupancy lookup.
The presentation translates that cursor with its target world. The minimap's
original six-by-six viewport rectangle is not resized. No mouse world-selection
transform or guest input rewrite is introduced. Original controller camera and
command bounds remain authoritative even when actors outside the visor are
visible. The analogous principle for a later DB2 task is to derive that game's
own native command scopes, not reuse DB1's dimensions or call addresses.

## Verification status

Pure geometry tests first failed without the new module, then pass45 width /
requested-offset combinations, including INT_MIN/MAX, clamps, exact8px steps,
320px original layout and inverse projection. Video-layer tests preserve the
existing camera, priority, weather, hazards and intro checks at their translated
native coordinates. New cases verify196 unchanged frame pixels, HUD edge323,
next-frame16px movement for offset+2, previously culled actors at maximum positive
offset, full top/bottom body continuations, and immutable guest RAM/VDP inputs.

Both CMake tests (`db1_video_layers_contract`, `db1_visor_contract`) pass.
Existing DB1 title geometry and the new branding intro-fade callbacks are preserved.

## Final native qualification

Final private executable SHA256:
`2c8257bcbcbd0c47cb3b618aa629828f5210eaa3ba7a4ec3645ec7842e31f52c`.
Nine real SDL2 hidden-window probes at2560x1440 produce94 actual427x240 captures.
The typed setting is loaded through `--visor-offset-tiles` and its real isolated
persisted profile, rather than patched guest memory. Summary and exact visual
measurements are in ignored `local/db1-visor-agent/final-summary.json`.

| Scene | Frames | Guest state | Audio state |
| --- | ---: | --- | --- |
| Static offsets0,-2,+2,-64,+64 |6800 each|`F381435D349C52E8`|`AC7965F65150A375`|
| Active Rain, offset0 |6800|`C66962717F9548D4`|`7D97EC69398CF3D7`|
| Native lightning, offset0 |8500|`9BA34BFE9337062D`|`816C73A9922AFD08`|
| Vertical camera/native-row continuation |6800|`15452FA9EBF9260F`|`62ED916A2D82E4AE`|
| Actor beyond original screen, positive clamp |6800|`AE9289FD4776275B`|`E39BCEEBCF79E4F4`|

Every state/audio pair exactly matches the corresponding original renderer/input
baseline. The offsets alter presentation, not gameplay or RNG. Default visible
white columns are64/259 (visor) and324/417 (HUD): exactly64 clear pixels on both
sides of the visor. Offsets-2/+2 move the visor strokes to48/243 and80/275,
exactly16px, while the minimap crop remains pixel identical in place. Requests
-64/+64 clamp to frame tile x10/x114 (applied-6/+7), retaining border/HUD gaps.
The complete208x208 frame crop is pixel identical for all five offset cases.

Active Rain frame018 has zero changed pixels between the original192x192
command interior and its translated wide counterpart `[66,24)..[258,216)`.
Lightning frame120 has zero changed pixels between the original320x64 cloud
band and the translated `[50,8)..[370,72)` band; no new cloud columns are added.
The original lightning's global palette remains authoritative outside that band.

The native actor-edge run uses ordinary A+RIGHT, no guest state fixture. At
sample36 the original320px renderer has no creatures on screen; the widened
renderer displays both complete bodies left of the visor, proving that host
actor visibility is not limited by the original drawing cache or frame. Sample42
naturally crosses the outer canvas edge. Native return-up14 additionally shows
continuous bodies through the added top rows and native-row boundary. Bottom-row
continuation and upper-ID faction coverage are established by the explicit
video-layer tests; this is not a claim that every campaign enemy was inspected.

These are bounded DB1 scenes and exact source/geometry contracts, not an exhaustive
campaign or physical controller/display/audio transport certification. Accelerated
dummy SDL audio hashes establish unchanged mixer state; queue drops in those
uncapped captures are not a physical playback measurement.
