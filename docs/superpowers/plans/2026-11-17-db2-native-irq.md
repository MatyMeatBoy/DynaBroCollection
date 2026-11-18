# DB2 native interrupt scheduling repair

**Goal:** Fix the reproducible first-gameplay unpause deadlock without changing guest code or enabling a 68000 interpreter.

**Evidence:** Benchmark and audio-enabled routes abort at the IRQ instruction guard, PC $0C0C polling VDP blank status. The full-program AOT path still calls handlers synchronously with raster progression gated and blank status forced true. DB2 can reach its raster wait from the V-int task callback.

**Architecture:** Only full-program AOT defers interrupt requests to completed instruction boundaries. The existing MC68000 step API owns exception entry, stack switching, mask/trace bits and RTE. Handlers execute as ordinary statically compiled instructions with normal scanline scheduling; no host CPU/frame restoration overrides guest context changes. Keep the legacy and bounded helper behavior separate.

- [x] Preserve two failing native probes and diagnostic state; identify the frozen raster wait.
- [x] Add failing pending-priority/mask and real core interrupt/RTE tests, including guest frame edits and USP/SSP.
- [x] Implement full-program-only pending request consumption and cycle accounting; update exact engine patch.
- [x] Reproduce successful actual gameplay after unpause, inspect captures and compare a reference route.
- [x] Run sibling DB1 IRQ/language/scenes, DB2 audio and replay controls; obtain read-only review.
- [ ] Qualify a new delivery identity. The closed 0.1.2 r1 stays immutable and gains a known-issue supplement.
