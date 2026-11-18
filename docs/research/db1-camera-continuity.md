# DB1 camera continuity and stage introduction

Scope: presentation only. DB2, guest RAM, guest RNG, guest input logic and guest
control flow are unchanged. Qualification uses the private translated ROM SHA256
`ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261`.
Final private binary SHA256:
`bb2f127361220d1dd00e1138db805e8f49a05ed6890ceec255edfbfe4237a00c`.

## Camera mechanism and final contract

The guest camera does not update its RAM fields atomically:

- `96C6..96D6` changes RAM scroll base200/202, before a VBlank wait at96DA.
- `96E8/96F6` commits actual plane-B horizontal/vertical scroll to the VDP.
- `9610/9650` changes coarse origins7BD4/7BD5; following instructions change
  circular name-table indices208/209.
- `966E/9678` sets fine animation offsets204/206. `96B0` later clears them.

Consequently fine RAM at raster line zero can describe a different camera phase
from the committed native plane-B scroll. Refreshing actors against live RAM on
every row also exposes intermediate camera phases within one body. A single
line-zero snapshot alone cannot correct a stale fine animation offset.

The final provider owns one RAM/terrain-VRAM geometry snapshot for the complete
frame, including both added canvas margins. It reconciles fine coordinates from
the scroll base and circular name-table position, rather than trusting204/206:

```
sx = signed_mod_512(RAM200 + 80 + 8 * RAM208)
sy = signed_mod_256(16 + 8 * RAM209 - RAM202)
```

These follow physical plane-B coordinates `x+80-sx+8*ring_x` and
`y+16-sy+8*ring_y`. The existing live VDP-minus-scroll-base displacement is then
applied exactly once. This handles a scroll-base RAM write awaiting its actual
VDP commit, a later coarse/ring change, and native quakes. Live CRAM, authored
hazard planes, raster UI and SAT effects retain their native presentation.
Top/bottom palette/effect snapshots remain owned; bottom geometry uses the same
frame RAM and terrain VRAM as the native rows, rather than live camera RAM.

Synthetic video tests cover both camera signs and axes, fine/coarse rollover,
occupancy/record changes at the next frame boundary, unchanged geometry during
mid-frame RAM writes, full-canvas joins, pending scroll writes, actual VDP commits,
forward/reverse ring updates and live palette changes. Existing intro, weather,
lightning, quake, UI and read-only checks pass. A private negative-control build
removing only camera reconciliation fails the new stale-phase assertion; the
final implementation passes `db1_video_layers_contract`.

## Actual native camera reproduction at1440p

`camera-vertical.input` uses ordinary A+LEFT, A+DOWN and A+UP to move two bodies
across the added top rows. It captures46 consecutive camera samples. No RAM
writes, states, injected PCs or simulation changes are used. The new native
`--window-size2560x1440` option requests a real hidden SDL runtime; screenshots
are427x240 at integer scale6, including8 added rows above and below native224.

Ignored evidence under `local/camera-continuity-agent/`:

- `vertical-integrated-1440/` reproduces incorrect actor phase with the earlier
  live-cache implementation. White egg body rows in return-up captures20..24
  are respectively59..65,59..65,43..49,75..81,59..65: a16px backstep,32px jump,
  then another16px backstep.
- `vertical-coherent-1440/` with the final binary yields59..65,59..65,59..65,
  75..81,75..81 in the same five captures: the native16px step with held frames.
  The body measurement uses white pixels within x248..264, excluding the frame.
- Top-margin transitions are visually inspected at down20/21 and return-up14/15/16.
  Bodies remain intact across the added8px/native-row join.
- `vertical-baseline/`, `vertical-integrated-1440/` and
  `vertical-coherent-1440/` all complete6800 frames with guest state
  `15452FA9EBF9260F` and audio state `62ED916A2D82E4AE`.

Other final1440p native runs:

| Scene | Frames | Captures | Guest state | Audio state |
| --- | ---: | ---: | --- | --- |
| `camera-motion.input` |6800|7|`934524752CFEEB7C`|`AA1E5CB94F4FA16B`|
| `camera-edges.input` |6800|18|`92426390A5F947FF`|`189E624B6C80226C`|
| `camera-vertical.input` |6800|46|`15452FA9EBF9260F`|`62ED916A2D82E4AE`|

All hashes exactly match the corresponding original binary/input run. The first
script covers all four A+directions; the second crosses the original208px seam.
Results are `motion-coherent-1440/`, `edges-coherent-1440/` and
`vertical-coherent-1440/`. The earlier224-row benchmark edge captures alone did
not expose the camera phase error; the full canvas vertical scene did.

## Stage introduction

Native baseline `intro-baseline-full/intro-36.png` shows narrow terrain during
fade. `intro-38.png` shows two eggs and split `STAGE01 S` / `TART`.
The actual guest initialization establishes these presentation phases:

- `1B0C: BSR33F2`, `1B10: BSR3472` initialize terrain/drawing.
- `1B38: JSR1B538` starts palette fade. The old scene hook1B42 is after its return.
- `1B70..1BF6` decodes falling egg tile5D6 and animates its native SAT entry.
- `1C04: BSR33F2` starts ordinary actor drawing after the drop.

Stage-intro classification now begins at1B38, so terrain is extended from the
fade's first visible appearance. Plane A is one centered320px foreground rather
than split at old HUD boundary208. The native falling SAT egg remains in world
coordinates; host actors are suppressed until1C04 to prevent a second egg.
Tests cover the two foreground pixels straddling208, one native egg, suppression
of the duplicate body and restoration of actors at1C04.

Exactly85 matching intro captures were generated before and after using ordinary
native input. Original runtime captures are398x224; final
`intro-coherent-1440/` captures are427x240. Inspected frames36/38/39/40 show
full-width terrain during the fade, intact centered text and one falling egg.
Both5600-frame runs produce guest state `3AC06ACE33C061D3` and audio state
`E2EE348212067CC1`. `camera-stage-intro.input` is the compact six-capture
reproduction; private `intro.input` captures every tenth frame through the intro.

This is finite native qualification of the stated scenes, not an exhaustive
whole-game or physical-audio-playback certification. The runtime uses dummy SDL
audio for accelerated capture; audio hashes establish unchanged mixer state.
