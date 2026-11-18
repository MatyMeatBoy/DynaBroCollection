# Coordination and handoff — 2026-11-18

Start here before changing this checkout. Active workspace: `D:/01Claude/Dynabrothers1+2`. Branch: `work/dynabrothers`. The working tree contains substantial uncommitted work from several owners. Do not reset, clean, stash globally, switch branches, regenerate patches or overwrite other owners' changes.

## Ownership board

| Owner | Scope | Reserved files | Status |
|---|---|---|---|
| Root/current chat | Integration, CMake, runner, SDK, packaging, launcher, shared hooks | CMakeLists.txt; games/step_adapter.c; games/checkpoint_launcher.c; tools/build.py; tools/native_probe.py; tools/package_checkpoint.py; upstream/segagenesisrecomp/**; upstream/recomp-ui/**; patches/** | Active |
| db1_world_geometry | DB1 continuous world, clouds, lightning, shake, HUD provenance | games/db1_video.c; games/db1_visible.c/.h; corresponding tests | Implemented; regression tests pass; integrated visual spell capture pending |
| db1_scene_widescreen | Native code/password contracts and progress persistence | games/progress_store.c/.h; games/progress_session.c/.h; corresponding tests; docs/research/progress-and-level-selection.md | Implemented; root integration pending |
| extras_guide | Pixel Extras compositor and guide content | games/extras_ui.c/.h; games/guide_content.c/.h; corresponding tests; docs/research/game-guide-evidence.md | Implemented; root integration and actual captures pending |
| DB2 helper (paused by user) | Current DB2 renderer integrated; no further DB2 work authorized for now | games/db2_video.c/.h; tests/db2_video_test.c; docs/research/db2-video-integration.md | Integration complete in source; full visual qualification pending; STOP DB2 development |

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


## Latest user steering

- User explicitly authorized a new test release once the principal audio/widescreen/control fixes are verified, including a portable ZIP. This supersedes the earlier note that publishing the next version had not been requested. Do all checks and publish when ready; no redundant permission question.
- User explicitly requested: integrate the current DB2 work and do not work on DB2 further for now. Renderer modules and root hooks are present; do not resume DB2 development or probes without user steering. Keep unqualified cases documented and avoid claiming all DB2 effects/scenes are finished.
- Shared settings now use a borrowed SDL renderer/window SDK path; this needs the exact SDK patch, full integration regression and release package evidence. Agent host-surface tests alone are not proof of the actual game modal path.
- Progress runtime/store/session/replay are now linked. Save Game action is in the host in-game menu. Saved levels are ROM-specific; native password-screen replay is queued without direct PC jumps. Native title-menu insertion and automatic screen navigation are still incomplete. Tutorials have text/examples; matching engine capture assets remain incomplete.


## Candidate verification update

v0.1.4 source candidate now includes all current modules. Seven real DB1 guide PNGs and their power-fixture provenance are present. Native lightning reproduction is fixed at the exact user-visible rectangle: see docs/db1-lightning-rendering.md. Shared host settings actual-game probe preserved window/renderer and CPU/RAM/VDP; controlled Quit intentionally exits an incomplete finite benchmark, reported separately. Original/enhanced1200 paced boot frames match state/audio and approximately20.029s against20.0258s ideal. Main launcher visually inspected through Computer Use with logo aspect preserved and both game cards; closed normally afterward.

Still required before publication: clean-source rebuild, package qualification (including sustained audio signal and native lightning), archive closure and uploaded digest verification. Full native level replay/physical controllers/audio and comprehensive scene acceptance remain separate limitations. DB2 remains paused.


## Published delivery — current authoritative status

v0.1.4 r1 is public and closed. Compiled source c3a5bdd9414c390483c8fd04f9b72f8e0ac818ad; archive SHA2563ee6dae01d3c1f0240e5c9aa0b642eb591a767f93176435be2c62f7d45423bba. See docs/releases/v0.1.4-verification.md. Final24 packaged cases pass, inventory unchanged, uploaded files downloaded anonymously and verified. Original0.1.3 archive unchanged. All helper tasks have ended; DB2 is paused. Earlier candidate-integration notes above are historical; use this section and verification document for delivery status. Remaining feature limits remain open, especially native title-menu Extras insertion, complete level-selection UX, unit numeric stats, full translations and physical hardware acceptance.


## Current integration — green hub and DB1 continuity (2026-11-18)

This section supersedes the historical ownership rows above. Public v0.1.4 remains immutable; the following work is a development candidate, not a released build.

- Root owns launcher/CMake/step adapter/SDK/runner/patches and delivery. DB2 game development remains paused by user instruction.
- Camera provider ownership returned to root. Native 1440p captures qualify frame-coherent terrain/actors and early stage intro; guest/audio hashes remain unchanged. See research/db1-camera-continuity.md. The measured defect was actor Y phase jumps, not disappearance of whole bodies.
- Submenu helper owns final native qualification only; sources returned. Verified authored checker is 16x16. Options/password repeat 128x256 with the original rolling glyph phase, live scroll and CRAM. Map and panel content retained pixel-for-pixel in previous native comparison; final corrected backgrounds are being qualified.
- Title art includes the approved English logo and generated missing bands only. Original center remains native. Full white/black fades verified; Japanese toggle preserves original logo. See research/db1-title-art.md.
- Five native disaster routes qualified at 1440p before the final camera change; rerun relevant integration evidence before release. See research/db1-hazard-contracts.md.
- Green launcher now embeds settings in its original HWND and renderer, with game art and controller navigation. Dyna theme removes CRT effects in startup and in-game settings. Real 125% DPI surface self-test passed. Exact SDK/engine patches refreshed; clean reconstruction remains a delivery requirement.
- Discovery validates ROM size/CRC, scans bounded conventional/configured paths, and inventories BPS/IPS. Arbitrary patches cannot be silently executed by static AOT. Visible patch inventory integration is in progress.
- Fresh integration tests: 36/36 native contracts, 43/43 Python tests. These checks do not replace native scene acceptance or physical M30/other-PC audio confirmation.

Still open: native title-menu Extras insertion, complete save/level-replay UX and qualification, verified user-facing numeric unit stats, missing English translations, complete achievement campaign acceptance, final portable release reconstruction and qualification. Do not report these as complete.

OptiPort freshness checked against canonical main a328c5a1b5aabc62f29aa9f1b5a96a59ad2d4ee1 on 2026-11-18.


### Integration closure before next release

37 native contracts are now registered. Final submenu captures qualify options/password and six map states at actual2560x1440; native content and guest/audio hashes match baselines. Native password replay loads Normal stage2; queued cancel and timeout release input. Save persistence and real Extras selection acceptance remain open. Read-only launcher review found explicit-ROM substitution and optional/platform target guards; both corrected. Three actual invalid explicit-ROM CLI cases return1 without launching another image; UI-OFF fresh configure passes. The patch inventory is informational, collapsed by default, and points to the built-in language setting; it never applies arbitrary external ROM patches. See docs/hub-mod-inventory.md.


## Authoritative current state after v0.1.5 publication

Public0.1.5r1 is CLOSED at source/tag19c9d879173173687a6587fcefbcf46dcc36987d; archive SHA2564b74f2668fdb2ac3e2bac9ec658238b677256e57f13288e6ebc3198f3962cb07.31 packaged checks pass, anonymous download/tag/digests/ZIP entries verified. Never replace its bytes. A separate private local personal archive includes the user's original dumps; never upload that archive or its ROMs. See docs/releases/v0.1.5-verification.md.

User moved original inputs to project/Dynabrothers/. This folder is private/read-only input and is ignored by Git. Root added internal-folder preference with legacy sibling fallback to build/qualification tools. Original images and original supplied patch match their recorded SHA256 identities.

Post-release source is ahead of0.1.5 and is NOT in its binaries. Root owns CMake/step adapter/extras compositor/launcher/patches; progress helper owns progress_ui/runtime and native qualification. New result-screen Save/Continue uses verified1BECC candidate plus stable1BD6C, physical START for Continue, and host-only framebuffer prompt. Functional EN/JA4:3/1440 and actual locked-file replacement failure pass. Composed visual captures are in progress; default raw screenshots exclude host UI, new opt-in host-overlay screenshots use a private copy and must retain guest hashes.

Unit guide now has five DB1 species pages with verified initial vitality, strict reproduction threshold/parent vitality cost and opponent-specific per-routine drain. This is not maximum HP, player power cost or DPS. Two-page Advanced tutorial now uses verified economy/route mechanics. Typed decoder and guide/Extras tests pass. Native guide visual qualification and a coherent source commit remain to do. DB2 content/development stays paused.

Remaining priorities: finish composed Save prompt acceptance, qualify hard-ending/other code-display lifecycle before claiming every-code Save coverage, add Extras naturally to native title/menu flow, complete level-selection navigation/initial-level UX, identify P-chan graphical Japanese bubble and any remaining English branches, finish broader achievement/scene acceptance. Physical controller/audio-device confirmation is distinct from automated evidence.


## Current verified development checkpoint — 2026-11-18 overnight

This section supersedes older pending items above. User authorizes autonomous decisions and continued work while asleep. Complete DB1 first, then apply verified learnings to DB2; DB2 is paused until DB1 readiness, not permanently blocked.

- Green hub Settings now passes actual native child hit-testing and BM_CLICK for both games, returns to the same HWND, and persists settings in an existing isolated profile. Default client is1100x620 DIP with smaller-workarea fallback. Combined approved1+2 logo is launcher-only; DB1 title retains its single-game logo.
- Original-size208x208 visor and192x192 command area are restored. Default427-wide view has equal64-pixel visible gaps. Signed settings move world/visor projection in8px tiles with whole-tile clamps; HUD remains right. Nine native probes/94captures verify actor visibility and original guest/audio hashes. Rain target crop matches native192x192 exactly; lightning retains authored320-wide visuals and native192x192 damage scope. See research/db1-visor.md and visor-settings.md.
- Ordinary result Save/Continue and timed hard-ending Save-only prompts are native-qualified, including held START, transitions, real persistence/reload and locked-file failure. Hard ending retains its original timer. See research/db1-progress-native-qualification.md. Native title-menu Extras and full level-selection navigation remain open.
- Title black outline and bottom-anchored original background are qualified. Only sky is extended above; generated ground is no longer sampled. White/black fades and Japanese mode retain original behavior.
- Story-only16px side fade is qualified over all224rows including lower text. Center288pixels, external margins and all three logo screens are unchanged; guest/audio hashes match baseline. See research/db1-intro-side-fade.md.
- Unit numeric guide is verified and committed206aeeb; five pages rendered and inspected. Unknown costs/speeds are not presented as verified.
- Integrated suites pass42/42 CTests and45/45 Python tests. OptiPort canonical main remains a328c5a1b5aabc62f29aa9f1b5a96a59ad2d4ee1.

These changes are development source, not the immutable public/personal0.1.5 archives. Next delivery must use a new version and exact packaged qualification. Remaining priorities: native Extras/title integration, complete level-selection UX, remaining English branches (including graphical P-chan bubble), broader campaign/achievement acceptance, portable checkpoint. Physical M30 and reporter-PC audio acceptance remain distinct from automated checks.
