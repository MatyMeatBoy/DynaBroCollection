# DB2 Special world geometry (2026-11-18)

Protected source ROM SHA256 `1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76`. All addresses below are DB2 observations, not DB1 constants.

## Authoritative terrain

- RAM `FF9FD0`: 40 columns x 40 rows, big-endian words; address = `FF9FD0 + y*80 + x*2`. The grid is storage extent, not a claim all cells are playable.
- `0057D8` loads the stage map stream selected at `FF027E`. `005802..0058D6` explicitly traverses 40x40 source bytes (neighbor row offsets +/-40); `0058DA..0058EA` widens 1600 bytes into this word grid.
- Low byte is terrain ID. High byte contains four 2-bit edge-overlay selectors, from bits7:6 down to1:0. RAM is authoritative after gameplay mutations.
- `015E9C`, `015EF8`, `015F58` emit a cell as 4x4 VDP tiles (32x32 pixels), into plane B at VRAM C000, row stride128, ring64x32 tiles.
- Normal ID mapping: quadrant q at RAM `FF2C0A + (id-1)*8 + q*2`; mask descriptor with511. Its 2x2 pattern-name words are RAM `FF340A + descriptor*8 + j*2`; add `6075` modulo65536 to obtain final VDP name word. Preserve normal VDP palette, flip, priority bits.
- High-byte overlays are faithfully implemented from `015FA8..016068` in `games/db2_world.c`. Selectors1/2 sample neighbor IDs at byte offsets[-80,+2,+80,-2]. Selector3 uses terrain9 and an address-derived choice. No RNG calls.
- ID14 hex is special (`016134`): seed is the sum of high and low words of guest cell address; four low-to-high nibbles index via `seed&7` the bytes at ROM1612C `[7,13,10,5,12,3,6,9]`. For each row, low-to-high bits choose background6075 or alternating037D/037C. This is source-verified but absent from the captured live map, so lacks live special-cell validation.

## Camera and projection

`FF2A56` is a long pointer to current camera record; captured value FF9C8E. Record bytes0/1 are coarse x/y cells (movement bounds1..33, proven `015AF8..015B38`). RAM FF2C08/09 holds plane ring tile origins, modulo64/32. FF2BF8/2BFA are signed horizontal/vertical scroll words. These are updated during scroll animation before coarse camera bytes advance (`015BE2..015C82`).

For world cell(x,y) and tile within that cell(u,v):

```
ring_tile_x = (ringX + 4*(x-cameraX+5) + u) & 63
ring_tile_y = (ringY + 4*(y-cameraY+1) + v) & 31
screen_x = (8*ring_tile_x + hscroll) modulo512
screen_y = (8*ring_tile_y - vscroll) modulo256
```

The camera cell anchor is `(8*ringX+160+hscroll) modulo512`, `(8*ringY+32-vscroll) modulo256`. Choose representative nearest(16,16) for native camera coordinates. The API exports these anchors and direct world-pixel projection. It reads RAM only and performs no guest mutations.

Captured camera(15,25), ring(20,16), hscroll208, vscroll-112 yields camera anchor(16,16). Base-only reconstruction first matched1408/1408 eligible resident names. Complete reconstruction, including high-byte overlays, matched2048/2048 resident plane-B names with zero differences. C implementation verified against the same captured RAM/VRAM with `terrain-test.exe base.ram base.vram`; built using GCC C11 `-Wall -Wextra -Werror`.

## HUD and lifecycle limits

`005354..005390` writes 13 columns x28 rows of HUD plane names starting E036 in normal layout: tile column27 / pixel216 through319. This proves a native right HUD boundary x216; other UI elements and game variants require their own gates. The renderer should preserve native composition there.

Battle wrapper arm003BB0 calls003DAC; return003BB4 begins postbattle handling; win003CB2. `003DAC` clears the broad world RAM arena before rebuilding it. Thus valid-looking terrain/camera RAM does not by itself prove a live battle. `FF027C/27D` are stage group/index set by006F9A, descriptor pointerFF0278 pointsFF0292. Descriptor and mode fields survive transitions and are insufficient as sole lifecycle gates. Decoder deliberately does not infer live mode: caller must provide a verified battle/tutorial lifecycle latch and disable during initialization/menus. World expansion outside the40x40 storage grid is unsupported.

Verified pause gate: 00425C sets bit7 of RAM FF0286 on entry; 004422 clears it after exiting the pause loop. FF006C points00442C while paused, cleared004418. Disable world expansion when FF0286 bit7 is set. Tutorial dialogue and full map overlay gates remain unverified.

Further modal evidence: 004148 sets FF0286 bit7 before calling full-screen00412C0; 0041BC clears after battlefield graphics rebuild. 009ED8..00A318 modal command handlers also set/clear bit7; 00C158 saves the old byte and sets bit7 during an actor effect, restoring at00C298. This flag is broader than pause. FF9C86 is conclusively battle outcome: 017E7E clears, 017E88 setsFF defeat, 017E94 sets01 victory. It must not be interpreted as a tutorial-text flag.

Native HUD geometry refinement: ROM config00505A loaded at004FE6 gives RAM FF2A5A=E036 (HUD plane origin), FF2A5E=00D8 (216 pixel HUD x), FF2A60=0010 (16 pixel world origin). Captured planeA E000 rows0,1,2,8,16,24,27 have names0000 in every column0..26 and HUD names in columns27..39. Thus x216 is the exact planeA HUD reservation; x208 is camera-origin16 plus six32px cells and is not a discovered clipping boundary. Actor renderer has no x208 clip; native planeA obscures actor pixels. A conservative compositor may fall back to native when planeA has nonblank/opaque content in columns0..26, to preserve unclassified world overlays/dialogue. This is a visibility check rather than a guessed mode-byte meaning.

Alternate game lifecycle:007304 calls00734E and returns007308; initializer007378 sets FF0286 bit3, sets up the same world grid, and alternates camera records in later updates. A3BB0-only battle latch intentionally excludes that path unless explicitly supported.

## Actor status markers and native effects

ROM014F22 has six8-byte tuples `(u16 bit,u16 attr,s16 dx,s16 dy)`:

| Byte28 bit | VDP attr | dx | dy |
|---|---|---|---|
|0|06EE|12|-4|
|1|06EE|12|-4|
|2|06EF|8|4|
|3|06F0|16|4|
|4|06EC|4|-4|
|5|06ED|20|-4|

Record offset28 is hexadecimal and is explicitly a BYTE load at014EEC. RAM FF2A89 bit0 suppresses all these markers. Coordinates use the same computed animation-adjusted body top-left at014E84..014EDA, then add table dx/dy. Each is a single8x8 VDP sprite, size0000. Loop014EF2..014F1A emits bits0..5 before the actor body, so these precede and obscure their own body and lower-listed markers. For host back-to-front compositing, draw body then markers5..0. These patterns are fixed, not part of dynamic actor pattern allocation. Minimal module `games/db2_markers.c/.h` exposes their native front-first sequence, with tests.

Global world marker06F1 is distinct: size0500 (16x16), emitted014DD8..014E2E before the first actor whose coarseY is <= marker coarseY, leftovers150B4. Its two temporary records at A5+4/+10 come from faction camera record fields16/17 when fieldF==0A (`015226`). Do not classify06F1 as peractor status. Global cursors/UI use earlier SAT-buffer entries and are built0150E0..01521E; preserve native ownership.

Pseudo effects use FFBCE6 +8*index. Their gather ownership word is FF00|index. Native source0147E0..0147F2 points their display slot at this record;014A14 sets pattern-base word to61F5+16*typeByte0;014E32..014E78 emits one32x32 sprite (size0F00) with final attr `(C1F5+16*typeByte0) modulo65536`. RAM FF2A89 bit3 suppresses pseudo effects. Preserve actual native SAT x/y rather than guessing their guest coordinate quirks.

Exact ground-depth association is available in the native gather list atFF9D3A: head word+0;8-byte nodes start at offsets4,12,...; node next+0, ownership+2, signed depth+4, previous+6. Ownership high byte is actorID orFF for a pseudo effect; low byte then identifies pseudo index. Actor depth at01F03A is32*u8(record3)+s8(record5); pseudo depth at01F0B0 is32*u8(pseudoRecord1)+s8(pseudoRecord3).01F0F6 and01F15A maintain descending depth, preserving the actual linked-list traversal for equal-depth ordering. Associate native pseudo SAT entries by active gather pseudo ownership, expected attr,size and traversal order; merge them into host actor depth instead of drawing all effects above all bodies. Native resident display-slot chain isFF20B2 (head) intoFF1BB2 entries32 bytes, nextword0; slot ownershipwordA, recordpointerlong1A. This chain preserves exact native actor/effect ordering when available.


Effect helper `games/db2_effects.c/.h` now exposes actual native-emitted pseudo SAT indices and gather depth/rank. Native gather count is72; only the first40 owners are display-eligible (0146FE). The helper validates the entire gather chain (bounds, alignment, cycles), walks at most80 unique SAT indices, matches exact pseudo attr and size0F, and assigns same-attr instances in gather/native order. Malformed SAT links stop at their valid unique prefix. A separate marker predicate masks VDP attribute bits. GLOBAL6F1 is explicitly unqualified and stays native because its coarseY comparison is not the same signed pixel-depth rule. Tests cover loops, bounds, same-depth ordering, output capacity, native40 cap, signed depth and input immutability; GCC C11 -Wall -Wextra -Werror passes. Standard VDP coordinates use X9bits, Y10bits, minus128 bias.
