> Current status: the renderer is integrated in v0.1.4. Further DB2 development is paused by explicit user request. The task below is a retained future handoff, not authorization to resume now. See docs/research/db2-video-integration.md for the actual limits.

# DB2 Special helper — concrete handoff

Read README.md first. Scope is game-owned DB2 work; do not alter DB1, shared runner/SDK, CMake, step_adapter, packaging or launcher while root owns integration. No helper has claimed this scope yet. Claim by posting owner/file list to current chat and updating the board. Existing pure decoder files are available and tested; do not rewrite from guessed DB1 offsets.

## First deliverable

Implement `games/db2_video.c/.h` and `tests/db2_video_test.c`: a genuine continuous read-only world provider using the existing decoders. Deliver CMake/step_adapter hook instructions to root as text, not concurrent edits. Test with a separate build/evidence directory. Root will integrate shared video canvas and dispatch.

Use the game_video.h provider ABI in the modified runner. Keep immutable per-frame copies of required state. Presentation must not change guest RAM, RNG, timers, actor logic, VDP status or native sprite limits in Original mode. Enhanced may render every visible actor, not just original culling limits, using authoritative actor records and animation state. Never copy the native HUD ARGB strip: it contains wrapped world pixels and produces duplicate dinos. Draw continuous world under the HUD, then only proven UI layers/sprites.

## Available decoded contracts

- `games/db2_world.c/.h`, `tests/db2_world_test.c`, `docs/research/db2-world-geometry.md`: terrain grid FF9FD0, 40x40 words, tables FF2C0A/FF340A; all 2048 resident tile words matched a real dump. Camera pointer FF2A56, scroll FF2BF8/FA, ring FF2C08/09. HUD starts native x216. Read full documentation for projections, boundaries and unqualified two-player paths.
- `games/db2_actors.c/.h`, tests and actor research: FF5F58 +48*ID, IDs1..254; occupancy FF8FAA; depth/gather FF9D3A; cached visual FF1BB2 with identity validation. Use native guest animation elapsed, not a new host timer. Supports multipart and compressed patterns. Host must remove original 40 visual budget safely while retaining depth/tie ordering and pseudo effects.
- `games/db2_markers.c/.h`: flags and offsets for actor status overlays. Draw reverse native-front order over body.
- `games/db2_effects.c/.h`: pseudo effects with SAT provenance, depth/rank, readonly bounded list traversal. Global 06F1 effect remains UNQUALIFIED; preserve native safe behavior until decoded.
- `games/viewport.c/.h`: shared strict presentation parser. Off/fit/stage/W:H; base398x224.
- Runner game_video optional canvas API computes integer fill. 1080p logical480x270 scale4, 1440p427x240 scale6, 4K427x240 scale9. Negative top/bottom rows must be actual world, not stretched screenshot repetition.

Normal battle hooks 3BB0 on /3BB4 off. RAM0286 bit7 is verified native modal/pause flag; conservative fallback while tutorial/dialogue unqualified. FF9C86 is battle outcome, NOT a general pause flag. Alternate two-player path734E is excluded until qualified. Global sim tick FFE066, no host speed changes.

## Evidence inputs

Ignored private inputs: local/db2-wide/base.ram and base.vram (65536 bytes each); local/db2-wide/baseline.input; local/evidence/db2-wide-baseline. Full 18000-frame route must be visually classified: tutorial/demo is not proof of normal campaign gameplay. A normal battle route used START at frames2000,4012,5000,6000,7000, DOWN5500, A7500 (12-frame presses); reference local/timing-db2-original-mode/frame-010000.png. Original native records and known actual patterns matched decoder output. Read docs before trusting any scene name.

Prepared DB2 English ROM local/roms/db2special-en-DAC9BAFC.md. Original SHA256 1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76. Do not publish ROMs or private evidence.

## Required regression evidence

- Original center identity and whole-view continuity, no duplicate body behind transparent HUD; actor at each edge/top/bottom, flight/multipart/depth/markers.
- More than40 visible actors (bounded to254); exact tie order and pseudo effects. World objects remain same in simulation.
- Camera wrap/edge, shake, rain/lightning and all spell effects continuous across original viewport boundary, not chopped at320. This is an actual DB1 field defect: proactively avoid same mechanism.
- Explicit safe fallback for menus/dialogue/unknown scene. Title/options/password/stage intro/gameover need per-scene extension, not unconditionally terrain overlay.
- English/Japanese and Original/Enhanced state parity for identical inputs, exact guest timing, no frame-dependent host simulation.
- Actual captures at1080p/1440p/4K plus deterministic frame evidence; separate renderer correctness from uncapped performance.

## Later DB2 work, coordinate first

Complete English prompts/text audit (there is a docs/research/db2-missing-mark-translation-candidate.md, not accepted translation proof). Refine custom achievements against true stage/mode/hard contracts, no invented unlocks. Guide unit numeric stats need ROM/manual validation; current guide has qualitative evidence only. Progress code hook58EF2 is owned by progress_session agent, do not duplicate or change it. Root owns shared Extras UI integration and captures; coordinate actual DB2 strategy-example capture scenarios from guide_content.c.

Deliver report: files changed, exact tests run/results, source/binary/ROM identities for native probes, known unsupported cases, root integration instructions. Do not claim whole DB2 done after pure decoder tests.
