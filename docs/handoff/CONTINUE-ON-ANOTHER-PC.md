# Continue on another PC — v0.1.8 checkpoint

Work is stopped at the owner’s request after this checkpoint. Do not assume the recompilation or translations are complete. DB2 feature development remains paused.

## Private development toolkit

Extract the complete toolkit into a writable local path. Run Restore-Workspace.cmd to verify every payload checksum, restore the source and all pinned Git dependencies offline, apply the tracked patches and restore private inputs/generated code. It refuses to replace an existing checkout. Check-Workspace.cmd additionally builds and runs the native contracts and Python tests. Open-Dev-Shell.cmd starts a shell with the bundled compiler, Git and Python; no global installation is changed. Windows x64 is required. Keep substantial free disk space for extracted tools, generated C and build outputs.

The restored checkout is workspace/DynaBrothers. From its development shell:

```
python -m tools.build --target both --jobs 6
cmake --build build/dev --target DynaBrothersLauncher --parallel 6
python -m tools.package_checkpoint stage local/candidate-next
```

The complete regeneration command is provided; relocation qualification builds the focused native contracts, not a full AOT regeneration. The ready-to-run personal ZIP is a separate artifact. Both contain private original dumps and must not be uploaded. Public releases omit those dumps.

## State and priorities

DB1 retains the v0.1.7 translation, native Extras, level selection and canonical progress saves. Saves contain validated native progress codes, not arbitrary snapshots. English defaults on; Original and Enhanced remain selectable.

DB2 now recognizes the actual C000/E000 plane pairing, uses a fixed 192x192 command visor and a common world projection, and preserves per-game visor settings in whole tiles. Six-button identification now uses guest-clock expiry and wraps the terminal handshake correctly. Native Normal2 inputs prove camera movement under the default six-button configuration. Original/Enhanced guest and audio states match over the finite 9400-frame fixture.

IMPORTANT: DB2 still falls back to the native image during some held camera pans and initial frames. Released camera captures have exact native world/HUD pixels at the expected offsets. Continuous widescreen scrolling, complete campaigns, all hazards, offscreen entity coverage and physical M30 variants are not fully qualified. Do not remove fail-closed sprite/cache guards to hide this. See private evidence/db2-visor-research/coherence.md for producer/SAT epoch research. No coherence latch has been implemented.

The older unpause-gameplay.input fixture follows attract/demo; it is not proof of player control. Use games/db2special/scenes/player-normal2.input for the qualified actual Normal2 input route. See docs/releases/db2-demo-fixture-scope.md.

DB2 translation remains partial. Private research inventories 35 additional renderer-linked Japanese strings and biography glyph resources. No biography overlay or bulk translation was applied. QUIT research is negative/unqualified. Start with the preserved evidence, not assumptions about scene identity.

## Ownership and reproduction

Root owns CMake, games/step_adapter.c, engine patches and final build integration. Give helpers separate modules or research directories; never let several agents overwrite shared adapters or build/dev simultaneously. No helpers remain authorized to continue work after this closure.

Exact source/dependency pins, private input hashes, tools and file inventory live in TOOLKIT.json and metadata/dependencies.json at the toolkit root. PACKAGE.json inside each runtime package records its compiled source and packaged checks. Private evidence is deliberately selected; old captures and generated data are not public source.

Physical controller recognition and reporter-PC audio still need testing on that PC. Include controller identity/mode, Windows audio device, scene and resolution with reports. In-game host menu is Start+Mode/Select, not Select alone.
