# DB2 active-gameplay interrupt repair

## Failure and root cause

Extending the paused tutorial route into active gameplay reproduced an abort in both benchmark and audio-enabled runs. The precise diagnostic stopped at PC `$0C0C`, polling VDP blank status, while the atomic host IRQ callback had forced blank high and disabled raster-budget yields. The callback also expected a return through its original stack frame and restored the whole host CPU image, which cannot represent arbitrary guest task/context switches.

Before evidence: `local/evidence/db2-unpause-guard-details` and `db2-unpause-audio-diagnostic`. This finding was made after closing the limited v0.1.2 r1 artifact. Its [known-issue supplement](../releases/v0.1.2-r1-known-issue.md) preserves the distinction; those bytes were not replaced.

## Contract and change

Only the full-program AOT driver uses the new path. Scheduler V/H requests remain pending until a completed instruction boundary. A pure priority/mask selection keeps masked requests asserted, prioritizes level 6 over level 4 and honors the H-int enable bit. The MC68000 core's interrupt-ack callback clears only the accepted live level, before frame bus writes, retaining lower-priority or subsequent requests.

The persistent main CPU context calls `m68k_step_interrupt`, which owns supervisor/user stack switching, trace clearing, mask, exception frame and 44 entry cycles. The original statically emitted instructions then execute the handler and its RTE in the ordinary cycle-scheduled stream. No whole-CPU host restoration overrides guest frame edits. There is no atomic-handler cycle debt/rebase in this path, and entry cycles are not charged twice. The legacy function-based and bounded native-helper paths remain separate. The new scheduler flag participates in lifecycle reset and scheduler snapshot metadata; complete AOT snapshots remain disabled.

The exact engine diff is recorded in `patches/0001-explicit-pc-mainloop.patch` and the existing source-pin verifier accepts it. No cartridge instructions or game data were changed by this repair, and no 68000 interpreter fallback was enabled.

## Evidence

- New `native_interrupt_boundary_contract` CTest verifies pending masks/priority and ACK isolation, STOP wake-up, 44 entry cycles, trace clearing, guest return-frame edits, USP/SSP switching and nested IRQ4/IRQ6 with normal RTE returns. The initial missing pending-policy implementation failed before the fix.
- Six native CTests and 38 Python tests pass, including a new rejection gate for a corrected v0.1.2 package missing active DB2 gameplay verification.
- The [unpause regression](../../games/db2special/scenes/unpause-gameplay.input) completes 18,000 frames in benchmark and audio-enabled modes. The final gameplay capture shows an active world and changing power/population, rather than only valid PNG files. Both runs use executable SHA256 `13061f6041ca8d90c5d377cb01b4a579bfe3a1fd4cd4ecd22a693bde7c4abff5` and agree on CPU state `2A11E9FEEB61F35D` and audio state `88AFA140AC25225B`.
- IRQ diagnostic logs confirm levels 4 and 6 with real frames and 44 entry cycles. The forced DB1 H-int route passes, both DB1 language/map routes complete 8,900 frames with expected English/Japanese captures, and a DB2 replay matches all 300 executions.
- Read-only review found no material flaw in the continuous-interrupt implementation and independently reran all six native contracts.

Accelerated dummy audio generates many output-queue drops; it tests control-flow/state equivalence here, not physical audio or presentation pacing. The independent reference remains useful for visuals, but its original input cadence reached different tutorial contexts; no frame-perfect equivalence to it is claimed by this test. Corrected packaged qualification and longer gameplay remain separate gates.
