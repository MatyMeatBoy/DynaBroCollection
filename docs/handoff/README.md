# Coordination and handoff — 2026-11-18

Start here before changing this checkout. Active workspace: `D:/01Claude/Dynabrothers1+2`. Branch: `work/dynabrothers`. The working tree contains substantial uncommitted work from several owners. Do not reset, clean, stash globally, switch branches, regenerate patches or overwrite other owners' changes.

## Ownership board

| Owner | Scope | Reserved files | Status |
|---|---|---|---|
| Root/current chat | Integration, CMake, runner, SDK, packaging, launcher, shared hooks | CMakeLists.txt; games/step_adapter.c; games/checkpoint_launcher.c; tools/build.py; tools/native_probe.py; tools/package_checkpoint.py; upstream/segagenesisrecomp/**; upstream/recomp-ui/**; patches/** | Active |
| db1_world_geometry | DB1 continuous world, clouds, lightning, shake, HUD provenance | games/db1_video.c; games/db1_visible.c/.h; corresponding tests | Implemented; regression tests pass; integrated visual spell capture pending |
| db1_scene_widescreen | Native code/password contracts and progress persistence | games/progress_store.c/.h; games/progress_session.c/.h; corresponding tests; docs/research/progress-and-level-selection.md | Implemented; root integration pending |
| extras_guide | Pixel Extras compositor and guide content | games/extras_ui.c/.h; games/guide_content.c/.h; corresponding tests; docs/research/game-guide-evidence.md | Implemented; root integration and actual captures pending |
| DB2 helper, unclaimed | DB2 renderer and game-specific research | See DB2.md; create games/db2_video.c/.h and tests/db2_video_test.c | Ready to hand off; claim before editing |

Use a separate build directory per helper (`build/db2-helper`), separate evidence/profile directories, and no concurrent links to root's build/dev executables. Read existing changes rather than assuming HEAD is the current implementation. Send integration requests to root instead of editing shared files. Root alone commits shared integration/pushes/packages. Record before/after evidence and leave a precise file list.

## Priorities and acceptance

- [ ] P0 DB1 lightning/clouds/shake must form one continuous view, no rectangular cuts, duplicated actors or copied terrain under HUD. Weather owner active.
- [x] P0 Audio Timer A failure identified and corrected in runner; integrated PCM probe passed: 88 active one-second windows of 102. Physical other-PC playback remains unconfirmed.
- [ ] P0 Recheck original/enhanced real-time pacing with exact 59.9227434043 Hz and default accurate pacing; preserve guest speed regardless render resolution.
- [ ] P0 Test physical and virtual controller flow, M30 shared-XInput identity, automatic source and six buttons; Start+Mode only for host menu, axes navigate without guest input leakage. Pure tests pass; root bridge integrated, native regression pending.
- [ ] P1 Genuine DB2 expanded world, actors and effects; see DB2.md.
- [ ] P1 Fullscreen integer-fill at 1080p/1440p/4K; scene/title/options/password/intro/gameover coverage. DB1 canvas and scene modules integrated; native visual qualification pending.
- [ ] P1 Extras integration: Level Selection first, Game Guide second. Pixel frames/font, controller navigation, two-page Basic and two-page Advanced tutorials, real engine capture examples. Numeric unit stats require verified sources/data. No invented statistics or captures.
- [ ] P1 Save progress option in every native code-display screen; atomic persistence; replay selection must preserve campaign save and use native validated contracts. Store/session owner active.
- [ ] P1 Same-window startup/in-game settings, game-themed bubbly launcher, approved English rock/volcano logo unchanged, CLI flags for RecompLauncher/EasySetup. Flag module exists; actual launcher dispatch and embedded renderer remain pending.
- [ ] P2 Complete missing English text in both games, particularly DB1; translation toggle enabled by default; keep partial status honest.
- [ ] P2 Achievements full DB1 catalog/events and DB2 Special relative custom set, badge assets and popups; previous progress exists, campaign qualification still needed.
- [ ] P2 Exact approved SDK patch reproduction, regenerate engine patch after owners finish, dependency provenance, README/badges and measured OptiPort feedback.
- [ ] Delivery: review, fresh build/tests, clean source checkpoint and independent portable qualification, local next test ZIP; do not overwrite v0.1.3 artifacts.

## Verified and protected

v0.1.3 public prerelease is immutable, source a314a74ea03de0bb62008830413df94f9ec2869b, archive SHA256 0da16abc82098b394d5ea2525eb287be26c5868a7ddc03f74b7f5726255e386d. It has known field bugs; never label its silence tests as audio proof.

Original dumps under `D:/01Claude/Dynabrothers` are read-only inputs, never write there or include ROMs in public source/archive. Local prepared ROMs live under ignored local/roms. No public ROM distribution authorized.

Current integrated probe: `local/audio-integrated-014/probe.json`, 6100 DB1 frames, default pacing request, sustained non-DC signal. Probe is uncapped and does not prove wall-clock timing. 43 Python tests passed. CTest integration now includes DB2 pure decoders, scenes, menu, flags, audio timer, guide; run fresh, do not reuse stale executables. No new release is qualified yet.

Build PATH: `C:/msys64/ucrt64/bin`. Use PowerShell. Current VERSION still 0.1.3. Planned test milestone 0.1.4, not yet a delivered release. Read docs/superpowers/plans/2026-11-18-db2-expanded-world.md and docs/research before implementation.

## End-of-pass integration update

The root integrated Extras as an action in the in-game host menu and linked guide/compositor into DB1/DB2 targets. DB1 target rebuilt successfully. This is not yet an original ROM title-menu insertion. Level provider is not wired; no native level launch is implemented. Save store/session modules exist but are not yet linked/initialized/called by step_adapter. Preserve this distinction when reporting status.

Fresh focused CTest run passed DB1 video layers (including weather regressions), menu controls, audio timer, guide content and Extras compositor, 5/5. SDK stb_image emits a GCC optimized stringop-overflow warning in PNG transparency code; investigate dependency version/contract before suppressing or shipping. Capture assets for guide remain absent.

### Suggested message to the DB2 helper

Read docs/handoff/README.md and docs/handoff/DB2.md in D:/01Claude/Dynabrothers1+2. Claim only the DB2 renderer scope in the ownership board, use build/db2-helper, and implement games/db2_video.c/.h with regression tests using existing verified decoder modules. Do not modify shared CMake, step_adapter, runner, SDK, DB1 or packaging; send root the needed integration diff/instructions. Prioritize genuine continuous 16:9 terrain, all visible actors/effects without duplicated pixels under HUD, preserving original logic and clock. Record actual native evidence, unsupported scenes, and precise test results. Do not publish ROMs or overwrite any release.
