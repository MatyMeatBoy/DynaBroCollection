# DB2 expanded-world implementation plan

**Goal:** Continue the project with DB2 Special expanded 16:9 as the primary deliverable, preserving DB1 and the immutable v0.1.3 package. Continue bounded translation/stability improvements independently.

**Design:** Retain the native AOT simulation, camera and input rules. Add a DB2-owned read-only renderer using authoritative terrain and actor records, original animation/pattern decoding, native raster layer provenance and a right-anchored HUD. Decode all valid visible actors independently of guest drawing-cache limits. Preserve complete centered native fallback for unsupported scenes. Default enhanced canvas is the nearest whole-pixel 16:9 width at 224 lines; original remains 4:3. Keep optional aspect parsing shared and transactional so both games have the same bounded configuration behavior. Do not copy DB1 addresses or assume its simpler animation format applies to DB2.

**Authorization:** The user has specified preservation, integer scaling, extra visible actors and default-on enhancements, and repeatedly requested autonomous progress. Work continues in the established development checkout with isolated test profiles; no additional design approval is needed. Existing closed archives are never replaced. ROMs remain protected outside public source/packages.

- [x] Refresh OptiPort: upstream and loaded modules match `a328c5a1b5aabc62f29aa9f1b5a96a59ad2d4ee1`.
- [ ] Record exact native baseline and RAM/VRAM evidence; map terrain, actors, animation formats, camera, HUD and lifecycle gates.
- [ ] Test and implement shared bounded viewport parsing without changing DB1 output/configuration semantics.
- [ ] Test bounded DB2 terrain/actor decoders, malformed streams, sentinels, depth/flight and immutable inputs.
- [ ] Integrate DB2-only extended composition, priorities, effects/markers, fallback and whole-pixel presentation.
- [ ] Compare identical Original/Enhanced workloads, canonical game/audio state and sampled original-area pixels. Check side actors, scroll, scene transitions and 1080p/1440p/4K geometry.
- [ ] Integrate separately verified English text improvements and retain transactional language toggling.
- [ ] Review, run the relevant native/tooling/regression checks, commit and push coherent changes. Prepare a new independently identified test package only after its promised scope passes; do not overwrite v0.1.3.

Research identification is not accepted implementation or whole-game equivalence. Code, fixture/native scene checks and package/hardware qualification are recorded separately.


## Additional user acceptance requirements — Extras and progress

- Add Extras naturally in the game menu, rendered with the game pixel font, palette and frames, usable with keyboard and controller.
- First entry: Level Selection. Second: Game Guide. Keep original gameplay and original mode behavior preserved.
- Offer Save Game whenever the original presents a progress code. Persist the native code with game/ROM identity, mode and level; reject corrupt or incompatible records. A progress-code save is not an arbitrary midbattle snapshot.
- Replaying stages must use verified native progression/password contracts and preserve the campaign save.
- Guide sections cover units with verified statistics/abilities, HUD, and hazards. Unknown numeric statistics must remain unpublished until verified.
- Basic Strategy and Advanced Strategy are each short tutorials of two pages, with concrete examples and screenshots from this engine. Advanced content excludes exploits and cheese.
- Screenshots must correspond to each example and be captured from the actual engine; no generated images presented as gameplay captures.
- Pending integration: native menu entry and navigation, progress capture and load, per-game unlock policy from original progression, capture assets and visual verification. Research/model tests alone do not complete this feature.
