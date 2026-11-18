# DB2 renderer integration candidate, 2026-11-18

Owned files: games/db2_video.c/.h, tests/db2_video_test.c, this document. Shared ABI/runner integration belongs to root. No release, ROM or package was written.

## Root integration

Link db2_video.c, db2_world.c, db2_actors.c, db2_markers.c, db2_effects.c and viewport.c into DB2. Include db2_video.h; return &db2_video through DB2 video selection. Call db2_video_observe(pc) at exact instruction observation and db2_video_reset() on reset/ROM change. Original mode stays native via configure off.

Required appended ABI: GameVideo.frame_complete(vdp,pixels,stride,width,height,native_top,native_height), invoked once after ALL native rows and after canvas_complete, including non-integer/native-height presentation. Root implemented this hook. Without it whole-frame fallback is not guaranteed. No runner pointers or native providers are retained.

Test sources: tests/db2_video_test.c plus the six sources above. Includes games, upstream/segagenesisrecomp/runner, runner/include, runner/video. GCC C11 -Wall -Wextra -Werror; assertions enabled. No link dependencies.

## Frame safety and continuous terrain

The 3BB0/3BB4 battle latch, FF0286 bit7 modal gate and bit3 alternate-mode exclusion qualify battle scenes. 734E clears battle ownership. Unknown scenes preserve the complete native image centered with backdrop margins.

All224 actual native ARGB rows are copied into owned storage. Every frame remains provisional until frame_complete. Any unsupported row, invalid actor, unknown sprite/global effect, opaque Plane A pixel in native world x0..215, or reconstructed/native Plane B mismatch rejects the entire frame. frame_complete then rewrites ALL rows to the captured native image, including rows drawn before rejection. Added rows become backdrop. Thus an unsupported effect never produces a half-native/half-expanded completed frame. No independent VDP evaluation or guest mutation is performed.

Terrain across the full view uses one projected40x40 authoritative grid. Native wrapped B behind HUD is never copied. Each native world pixel0..215 is compared with the same terrain reconstruction used beyond216. Unknown weather/scroll effects that fail this check cause full-frame fallback. Palette/VDP snapshot updates every row; RAM changes recapture camera and actors. Unknown shake/weather continuity is handled by fallback, not claimed decoded. Palette-only effects use current CRAM across the entire row. Added vertical rows use the latest owned completed state.

Only Plane A HUD provenance216..319 relocates to the rightmost104 columns, retaining transparency over actual world. No native ARGB strip is copied.

## Sprite provenance

ROM155CE/15648 compacts94 source sprite records starting FF2752 into SAT, skipping zero attributes. Source A5 is FF056E. Native UI is recognized by exact source RAM record, full attribute, size, X and Y match, NOT by its current pixel position:

- Source records0..5, attr86E7 and size0 are HUD counters. Writers691A..694A and6CBC..6CF0 establish their ownership; these relocate horizontally with the HUD.
- Records6/7, attr82F4/8376 size0A/05 are native world cursors from150E0..1517A. They retain world coordinates.
- Records8..11, attr9EE8/8EE8/96E8/86E8 size05 are frame corners from694E..698C. Records10/11 (right corners) relocate; left corners retain X.
- Record31, attr032F size0F is selection from1517E..1521E; it retains world coordinates.

Every active linked SAT entry must be classified, including offscreen entries. Unknown entries (including global06F1) reject the whole frame. Native actor bodies and peractor markers are replaced by decoded host actors. Pseudo SAT effects are matched to verified gather depth/rank. No unknown native effect is assumed to be HUD.

Actors1..254 use guest animation elapsed, cached native visual when identity matches, otherwise pure decoder. Any populated undecodable actor rejects the frame. Bodies and markers preserve native group ordering, depth/ties merge actual native pseudo effects. Pseudo synthesis beyond the first40 native display-eligible owners remains unsupported.

## Verification and limits

Strict GCC C11 -Wall -Wextra -Werror object and test builds passed in build/db2-helper. Test output: `DB2 video synthetic terrain/HUD/fallback/snapshot/canvas contracts passed`.

Tests cover genuine terrain under transparent HUD, modal/unknown Plane A fallback, reset/alternate scene, immutable added rows, 1080p480x270(scale4),1440p427x240(scale6),4K427x240(scale9), actor41 outside native width without SAT/cache membership, per-row palette continuity, late unknown06F1 whole-frame rollback, and native terrain mismatch rollback.

Native probes/captures are still pending. Synthetic tests do not prove actual normal campaign ownership, multipart/flight/overlap appearance, all effects, language parity, wall-clock timing or physical presentation. Extra world rows currently use the latest frame snapshot; a frame with raster palette changes can produce a different palette on its added rows. Title/options/password/intros/gameover have centered native fallback only. Unknown weather/global06F1 deliberately lose expanded-world geometry for their entire frame. This is a conservative candidate, not a release qualification.
