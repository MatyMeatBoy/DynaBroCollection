# DB1 frontend and stage introduction presentation

Scene classification follows actual guest routine entries, rather than the
colors or text of a screenshot. Addresses are in the original Japanese DB1
ROM's 68000 address space; the translated ROM preserves these code addresses.
The source was inspected with Capstone in `local/world_scan.py`.

| Entry | Scene contract and evidence |
| --- | --- |
| `1C5A4` | Branding sequence. Clears CRAM and name tables, loads the first logo from the resource directory at `70004`, presents/fades it, then loads the next logo. White margin fill belongs only to this classification. Native center stays unchanged, including its fades. |
| `1C9CE` | Title background setup: calls `1D1F4`, then `1CCC8` (scroll register `8B02`, resource loads), followed by title drawing and input loops at `1CA4A`. |
| `1CAF6`, `1CB0C` | Return to title after demo/menu paths, calling `1ACD8`, `1CCC8`, and menu routines. |
| `1E024` | Options input release loop, then `1D1BE`, `1ACD8`, resource decode `6FC00`, options graphic setup. Called from menu selection 2 at `1CC80`. |
| `1D7B6` | Password input release loop; initializes `FF0036` to `6A6A6A6A`, decodes `7C10`, and builds the password panel. Called from menu selection 3 at `1CC96`. |
| `1950` | Stage initialization; invalidate frontend before loading game data. |
| `1B42` | Terrain and actor drawing initialization has completed (`1B0C: BSR 33F2`, `1B10: BSR 3472`) and palette fade (`1B38: JSR 1B538`) has returned. The native START sprite animation is about to begin; terrain extension can use the existing world sampler here. |
| `1C40`, `1C44` | Enter/return from battle routine `1D06`. |
| `1BC04` | Explicit losing outcome in the result-screen routine. `1BB40` captures `FF003D`, `1BBC0` loads it, and `ADDQ.B #1` / `BEQ 1BC04` selects precisely `FF` (negative losing outcome). It calls `1C376` and `1BEA8`, selects music `0E`, and draws resource/text at `1C29E`. Thus GAME OVER is classified by control flow, never by arbitrary dark gameplay pixels. |

The frontend extension reads actual plane B name-table entries, pattern bytes,
palette, flips, priority, horizontal scroll mode/table and VSRAM. Signed native
screen coordinates continue the name table's hardware wrap; no generated terrain,
scaled logo, repeated foreground panel, or sampled screenshot is used. A 64-tile
plane can therefore expose its existing 512-pixel width beyond the native 320.
The center 320 pixels are copied verbatim from the native renderer, preserving
its plane A, sprites, window, priorities, and mid-frame changes.

Additional vertical rows use the nearest native raster's horizontal scroll entry;
two-cell vertical scroll outside the native horizontal span uses the nearest
native column. Pattern/name-table coordinates themselves continue normally.
Reserved plane sizes and interlace mode 2 return transparent rather than guess.
Snapshots used for completion must include registers, CRAM, VRAM **and VSRAM**;
`db1_scenes_background_for` accepts a captured scene enum to avoid consulting
live scene state during frame completion.

`tests/db1_scenes_test.c` checks tilemap wrap, both flips, scroll provenance,
native-center identity, immutable VDP input, scene hooks and branding isolation.
