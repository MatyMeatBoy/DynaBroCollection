# DB1 native result Save option

User authorized autonomous completion and original-style extra options. v0.1.5 is immutable at19c9d87; this work follows that checkpoint.

## Design and boundaries

Keep native result code/statistics/sprites intact. At the verified result wait loop, replace only PRESS START prompt with a small original-style pixel panel: CONTINUE, SAVE GAME, and save feedback. Default CONTINUE; release-before-accept. Saving persists the verified displayed code atomically and remains on the result. Continue injects a bounded physical START pulse through the existing input path; no PC/stage/ROM/VDP changes. Animations continue because visibility and pause state are separate. Original4:3 and enhanced16:9 share the centered native320x224 viewport transform.

Activate only for a verified1BECC result candidate after stable input hook1BD6C. A1DD90 accepted-password candidate must never activate the panel. Clear at1BD7C native exit and native initialization/title hooks. Practice congratulations and hard ending need separate verified lifecycles; do not guess coverage.

## Files and tasks

1. games/progress_ui.c/.h and tests/progress_ui_test.c: pure lifecycle/input state. Tests first cover early drawing, repeated stable hooks, password exclusion, held-button release guard, two choices, save result/retry, bounded Continue and reset. Abstract buttons avoid SDL/CPU dependencies.
2. games/progress_runtime.c/.h: own state, observe verified PCs after session capture, convert pad masks, invoke a shared save helper without opening Extras for the inline action. Preserve existing host-menu Save behavior and dormant qualification diagnostics. Expose read-only prompt state.
3. games/extras_ui.c/.h: reuse the existing pixel glyph/drawing primitives for a160x45 black/white/cyan prompt at native(80,176). Preserve the upper code/statistics and sprite row. Clamp writes and validate pitch/viewport.
4. games/step_adapter.c and CMakeLists.txt (root only): link state module, include prompt in host_ui_active but not host_ui_open, render prompt before other overlays. Compile and run focused contracts.
5. Native isolated prepared-result fixture: capture idle/select/save/continue at4:3 and1440p16:9, English and Japanese; verify written code/store and native exit. Setup is diagnostic, not a campaign completion claim. Test failed write without losing previous save and post-menu release. Review actual frames before commit.

No DB2 game changes. No native title-menu Extras insertion is implied by this result-screen feature.
