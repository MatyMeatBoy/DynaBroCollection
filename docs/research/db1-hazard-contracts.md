# DB1 native hazard presentation contracts

Scope: DB1 only. Addresses are the original guest instruction addresses, verified
by disassembling the local DB1 image; translated labels do not move code.
`db1_hazard_effects` is a pure host observer: it accepts PC events and changes
only its own presentation flags. It does not write RAM, VDP, RNG or guest PC.
Observe the exact entry and return instruction before executing it. Clear all
flags on reset/battle teardown. Do not use the currently executing PC range as
an activity test: native VBlank and shared drawing helpers interrupt effects.

## Complete disaster submenu

`4682` contains five long pointers. Selection index is `4*y+x` (`296C`), with
four columns: indices 0..3 on the first row and index 4 on the second row.
`46C6` invokes the selected routine through the native controller path.

| Index | Player command | Native presentation entry / return | Layer contract |
|---|---|---|---|
| 0 | `120F4`, lightning | `132D8` / `13630` | Authored plane A at C000 over the full output, including the normal HUD region; cell-row horizontal scroll. |
| 1 | `1205E`, distortion (name not yet qualified) | `15D2C` / `15E52` | Native row scroll on both planes and global CRAM flash; reconstruct terrain using each row's actual B scroll and use live CRAM. It does not replace A with a new full-screen artwork. |
| 2 | `11FDA`, earthquake | `12FE6` / `13286` | Both VDP scroll planes and native SAT shift. Apply the actual scroll-to-camera delta to expanded terrain and actors. |
| 3 | `12208`, tsunami | `14070` / `1429C` (populated); `1429E` / `144F6` (empty/surfer) | Authored full-width A at C000, with native scroll and priority. Surfer sprites are screen anchored. |
| 4 | `12178`, meteor | `5650` / `5D62` | Native screen-space SAT projectile composition plus B shake; preserve full sprite geometry and native order. Do not reinterpret these as creature records. |

The tsunami A4 value E000 is a **tile attribute**, not the plane B address.
`14698` writes the VRAM command words 4000/0003 (C000). `14674`, `146D6`
and `147F2` author wave names there; `1483A` changes A horizontal scroll.
Artwork streams are `15110` and `15A4C`, loaded to VRAM 2A40 and 38C0.
Both tsunami returns follow restoration/cleanup; use the separate returns
rather than assuming the first variant's entry ends the other variant.

Distortion's `15FC6` enables register 11 value 07 and fills vertical/horizontal
scroll data; `15D70..15DA4` writes paired per-row offsets. `15DDE..15DF0`
writes white CRAM; `15DFA..15E3C` restores scroll mode and camera scroll.
`120D4` subsequently calls `15E82` to fade the palette back, ending at `15EBE`.
Live CRAM must therefore remain authoritative after the distortion flag clears.

Meteor's `5C96..5CBE` emits native SAT coordinates with the hardware 128 bias.
`5CCC..5CEE` adds shake to B scroll; `5D0E..5D2A` restores it. Its authored
spawn range is native-width screen space. Widening spawn coverage would change
the guest simulation/RNG: presentation must not invent additional projectiles.

## Persistent weather and local spells

Rain `11ECC` checks/charges power and calls `12A5C`, which sets bit 2 of
FF0168 and duration FF1812. The other faction's corresponding list uses bit 3,
FF1813 and `12A80`. Lists FF181A and FF1864 are authoritative world-cell pairs
with FF terminators, not repeating screen textures. Extend only actual listed
cells; keep their world anchors and native alternate-art selection (bit 6).
`6372`/`65F2` perform native viewport culling; the expanded renderer must visit
the complete bounded lists independently. Clearing those bits ends weather.
Rain is an area command over six-by-six cells, not permission to fill the
entire expanded world with clouds. Dry/plant/terrain mutations must render from
the resulting authoritative world grid; no invented weather is warranted.

Shared spell presentation `5FF6` / `62FC` uses native SAT plus B displacement
(`61DA`). Preserve linked first-wins SAT order, art even when tile numbers
overlap HUD tiles, and priority. Local spell sprites remain anchored at their
authored world/screen coordinates. Never stretch individual sprites or add
copies to fill a wider viewport. Global palette flashes naturally apply to
every output pixel through live CRAM.

## Verification and capture limits

The pure classifier test covers all paired entry/return events, retention
through VBlank, simultaneous independent flags, reset, and all 256 weather
flag values. It is not evidence of integrated video correctness.

`hazard-{lightning,distortion,quake,tsunami,meteor}.input` use the existing
native lightning route, with only controller movement inside the real submenu.
The sole write is the explicitly labeled resource fixture
`WRITE_RAM32 0xFF7BD6 0x00999999`. No PC jumps, weather writes, actor writes,
ROM changes, or dispatch replacement are used. These are diagnostic examples,
not earned campaign progress. Captures must be inspected and the actual
selection verified before being described as native hazard qualification.

Baseline diagnostic runs on 2026-11-18 used a private executable copy with
SHA-256 `38bfc2432141feb97a35dbff770c0cb8576d3dc7d3b2bbf2ca9cd86f9ca5f5f2`.
Each of distortion, quake, tsunami and meteor completed 8500 guest frames,
produced eight captures, and reported zero AOT miss events. Evidence remains
local at `local/evidence/db1-hazards-before/`; it is not a ROM or release asset.
The inspected tsunami frame 300 shows the native wave over the output except
for the erroneous old-terrain strip at x208..285. Meteor frame 120 shows the
native falling projectiles; distortion frame 120 shows its rippled global
white flash. These baseline images establish actual native dispatch and the
tsunami seam; they do not establish that the integrated fix has passed.

### Integrated native verification, 2026-11-18

The integrated private executable SHA-256 is
`9b5940e0db5fc709a33e539126f1cf375101177a386e7be0ed8ce4eee7d2d681`.
All five submenu scripts completed 8500 benchmark frames without AOT misses.
Their final canonical state and audio-state digests match the private baseline
exactly:

| Native menu entry | Canonical state FNV | Audio-state FNV |
|---|---|---|
| Lightning | `9BA34BFE9337062D` | `816C73A9922AFD08` |
| Distortion | `EC703A05AF30744B` | `F7DC70CFD2C2003D` |
| Quake | `18925628C38CD04F` | `E2705EBF5346B9A6` |
| Tsunami | `B9120516558C57F2` | `834D3C45057870B1` |
| Meteor | `1C10E6295972C0E7` | `8CEC27BDBA78AC6E` |

The inspected corrected tsunami frame 300 has continuous water across the
398-column output. Compared with baseline, its first 208 columns are pixel
identical; the former HUD rectangle x208..285 changes at 14812 pixels.

Separate actual SDL `--window-size 2560x1440` probes produce a 427x240 logical
canvas. Inspected lightning frame 120 has continuous clouds across that full
width and native bolts; tsunami frame 300 has continuous water across all
427 columns and all 240 rows. Distortion frame 120 preserves the live white
flash and native per-row ripple. Quake retains the battlefield and separate
HUD with native camera displacement. Local evidence is under
`local/evidence/db1-hazards-after/1440-*`. The first lightning probe retained
only the last of two same-frame screenshot requests; the redundant menu
request was removed without changing any input or frame timing, and
`1440-lightning-r2` passes the complete capture check.

All five actual SDL probes pass, contain non-DC mixer PCM, and end with the
same respective state/audio-state hashes in the table above. Meteor frame 120
was inspected: its native falling projectiles render over the expanded world
and its normal HUD remains natively hidden. The runtime summary is
`local/evidence/db1-hazards-after/1440-summary.json`. Uncapped dummy-device
audio runs report queue drops; these WAVs establish mixer signal and state
parity, not physical playback or real-time underrun qualification.

This qualifies the five actual disaster submenu entries, not every possible
local spell, enemy branch, or alternate weather-art state. Rain's world-cell
policy and local spell handling are separate contracts. Diagnostic power is
still the only guest-memory fixture; no result claims earned resources.
