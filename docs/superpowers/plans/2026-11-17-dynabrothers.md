# Dyna Brothers Implementation Plan

**Goal:** English translation first, followed by a verified Windows native port.
**Architecture:** Preserve immutable Japanese inputs; generate hash-qualified English copies. Pin GenesisRecomp and integrate game-owned adapters after baseline validation.
**Tech Stack:** Python 3 standard library, C11, CMake, Windows x64.
**Spec:** AUDITORIA-INICIAL.md. English confirmed; continuous execution authorized.

## Constraints
- Translation enabled by default and independent of Original/Enhanced mode.
- Original preserves 4:3 and gameplay; Enhanced extends the world without changing simulation.
- Three/six button support, six default. No inert advertised features.
- Proprietary ROMs/generated source remain local and ignored.

## Review focus
- Wrong ROM and corrupt/truncated patches must fail before output is written.
- IPS cannot prove source identity; DB2 Special requires independent qualification.
- Preserve original files and Japanese recovery path.
- Discovery coverage is not gameplay correctness.
- Non-square pixels require explicit aspect treatment.

## Tasks
- [x] 1. Implement bounded BPS/IPS parser and hash-qualified local preparation in tools/rompatch.py; test real DB1 CRC plus malformed synthetic inputs in tests/test_rompatch.py.
- [ ] 2. Identify IPS changes and edition compatibility; produce local/evidence/patch-analysis.json and translation coverage inventory. Validate both boot and translated scenes before declaring support.
- [x] 3. Build pinned recompiler; game-owned configs under games/; generate original and translated targets independently; record discovery and limitations. Explicit-PC AOT replaces incorrect function-based bring-up; see STATUS.md.
- [ ] 4. Integrate native adapter and runtime with deterministic boot/scene probes. Compare original reference, audio and input.
- [ ] 5. Recover text/font/graphics formats; translate/reinsert missing English with context and visual validation.
- [ ] 6. Shared launcher/in-game configuration, modes and controls; validate runtime changes and saves.
- [ ] 7. Extend rendering and visibility for 16:9; test 1080p/1440p/4K and integer thresholds without simulation changes.
- [ ] 8. Package, regression, notices and honest acceptance report.

## Ledger
2026-11-17: workspace was empty except initial audit; no pre-existing game code. Upstream cloned at 4b2247d2b33a45623a686043a48c2021744c39ab. Toolchain found in C:/msys64/ucrt64/bin. All gameplay work remains unverified.

2026-11-17 milestone: English confirmed and continuous execution authorized. Bounded patch tools and native 68000 bring-up implemented. DB1's supplied translation reaches first gameplay in EN/JP AOT; DB2's new 22-prompt partial translation reaches early tutorial gameplay. Original Special retail IPS was proven incompatible and never adopted. H-int/source-pin review findings fixed with regression evidence. Full translation, timing/campaign equivalence, launcher, in-game toggle, modes and widescreen remain open in STATUS.md. No completion claim for the overall project.
