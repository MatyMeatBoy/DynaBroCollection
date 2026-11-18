# DB1 local achievement contracts

The native port implements the stated goals of all 76 achievements in the public [Dyna Brothers catalog](https://retroachievements.org/game/5362). This is an independent local implementation. The authenticated RetroAchievements evaluator definitions were unavailable; this work does not establish exact evaluator equivalence and does not synchronize RetroAchievements accounts.

The public catalog supplies achievement IDs, descriptions, points and badges. Goal interpretation was checked against read-only disassembly of the Japanese game and the project's English text patch. The relevant code ranges described below are byte-identical between those two inputs. This document contains addresses and descriptions, without ROM data or private screenshots.

## Observer and integration

`games/db1_achievement_events.c` is a pure observer. It reads a big-endian 64 KiB RAM image corresponding to `FF0000..FFFFFF`, the main CPU's PC before execution, eligibility, and the CPU A2 register. It modifies only its own state and emits public achievement IDs through a callback. Persistence, duplicate suppression, popup presentation and ROM identification belong to the caller.

Call `db1_achievement_events_observe_cpu(state, pc, ram, size, eligible, a2, emit, user)` before the following instructions:

| Purpose | Before-PC addresses |
| --- | --- |
| Battle and result | `001C40`, `0028AE`, `001C44` |
| Rain | `011EF4` |
| Earthquake, Tsunami, Meteor | `011FF6`, `012224`, `012194` |
| Enemy removal counter | `00D37A` |
| Timeout extinction | `00A10E`, `00A13C`, `00A162` |
| Surf event | `01429E`, `014070`, `014566` |
| King Tyranno evolution | `00C7A6`, `00C7B8` |

The older `db1_achievement_events_observe` wrapper remains compatible, but cannot qualify King Tyranno because it has no live A2 input.

Eligibility must be false for unsupported ROMs or runs affected by debug/test injection. An ineligible or truncated observation resets the current battle. Arming happens at the actual battle call `1C40`; descriptor mutation cancels the observation. Loading an already completed state without seeing the required event sequence does not qualify a goal.

## Battle and stage qualification: 68 goals

The campaign menu at `1CC16..1CC3A` derives a stage index from mode times 20, stores it at word `FF0034`, and sets descriptor pointer `FF002E` to table base `FFADDE` plus index times `4A`. The password parser at `1DCEC..1DD8A` independently uses the same descriptor stride.

The observer verifies a canonical descriptor pointer and its ordinal byte. Practice uses indices 0–7 with ordinals 1–8; Normal uses 20–39, Hard 40–59, and Back 60–79 with ordinals 1–20. Campaign stageword and descriptor must agree. Practice deliberately uses the pointer: `1C90` advances the practice descriptor without incrementing `FF0034`. Reserved indices 8–19 are rejected. Special password descriptors 80–96 have ordinal zero and do not produce numbered stage clears.

The battle routine called at `1C40` returns at `1C44`. Its loop at `288A` runs the simulation, checks result byte `FF003D`, and returns at `28AE` only when it is nonzero. The result-producing code writes loss `-1` at `A0F2` and victory `1` at `A104`; the results screen also explicitly recognizes `1` at `1B9EC`. The observer requires the real battle result return with `1`, followed by `1C44` with `1` and the same descriptor.

Final-stage descriptor types branch away from the ordinary results UI at `1C9C`, `1CC0` and `1CE4`. Therefore qualification uses the battle return rather than requiring results-screen entry `1B9DA`.

| Public goals | Emission |
| --- | --- |
| Practice 01–08 | IDs 183303–183310, corresponding practice index |
| Normal 01–20 | Catalog IDs corresponding to Normal stage number |
| Hard 01–20 | Catalog Hard ID and corresponding Normal ID |
| Back 01–20 | IDs 183283–183302, corresponding Back stage number |

Hard also awards the corresponding Normal goal because its public description explicitly accepts Normal or Hard. Sega's [original instructions](https://vc.sega.jp/vc_dyna/instruction.html) independently document eight Practice and twenty Normal/Hard stages.

## Special goals: eight contracts

| ID | Public goal | Verified local event |
| --- | --- | --- |
| 193201 | Rain 100+ times, then win | Successful player Rain hook `11EF4`, saturating count 100, followed by verified victory |
| 193202 | Defeat 100+ enemies, then win in Normal/Hard/Back | At least 100 observed single increments of the game's enemy removal counter, followed by verified victory in index 20–79 |
| 205018 | Use Earthquake, Tsunami and Meteor, then win | All three successful player cast hooks in one battle, followed by verified victory |
| 193203 | Trigger extinction by timeout, then win in Normal/Hard/Back | Observed timer expiration, first extinction flag and completed disaster dispatch, followed by verified victory in index 20–79 |
| 205017 | Evolve a player Tyranno into King Tyranno in a numbered stage | Actual fourth carnivore victory's King marker transition in the live player actor record |
| 193204 | Win PUGU | Verified victory using descriptor index 92 |
| 205061 | Win PPPP | Verified victory using descriptor index 87 |
| 193205 | Surfing Rex | Successful player Tsunami enters the empty-unit branch and reaches the surfer animator |

### Successful casts

Rain routine `11ECC` checks sufficient player power through `128D4`, rejects an already active rain flag, and charges through `12914` at `11EF0`. Only success reaches `11EF4`. Its terrain helper `10B6C` grows existing grass or creates grass on valid soil; `12A5C` starts its timed rain flag. This matches the Rain behavior described in Sega's original instructions. Failed commands do not count.

The three disaster hooks are likewise immediately after successful power checks and charges. Earthquake `11FF6` calls `12FE6`, which shakes both scroll planes with alternating random offsets. Tsunami `12224` leads to wave animation `14070` or `1429E` using the original wave artwork. Meteor `12194` calls `5650`, whose projectiles enter from negative Y and fall diagonally; the subsequent area effect invokes destructive terrain helper `11298`. Enemy casting uses separate paths and does not satisfy these player hooks.

### Enemy removals

Initialization `D182` clears the actor tables and both faction counters; `D1C4` stores zero at long `FF258C`. The player counter occupies word `FF258C`, and the enemy counter word `FF258E`.

Removal routine `D2AC` calls `D352`. Actor ID bit 7 chooses the enemy word; `D374` increments the selected word up to 999. The removal subsequently clears the actor type through `D3AC` and recycles its ID. `D37A` is observed after the counter update. The observer requires an initial enemy counter of zero and counts only witnessed increments of exactly one. A counter discontinuity disqualifies this goal for the current battle; a pre-existing value of 100 cannot unlock it. The local interpretation uses the game's own faction removal statistic.

### Timeout extinction

Simulation entry `A032` establishes A6 as `FF252C`. At `A10E` the countdown is long `FF7BB4`. `A134` checks first-event flag `FF7BC9`; `A13C` sets it and the code dispatches the descriptor-selected disaster at `A15E`. `A162` follows its return.

Qualification requires timer exactly 1 and flag clear at `A10E`, timer zero and flag clear at `A13C`, then timer zero and flag set at `A162`. A timer initialized at zero, an already set flag, or a practice/special stage does not qualify.

### Password battles

The public password table at ROM `1E87E` contains PPPP at index 87 and PUGU at index 92. The parser compares the entered password directly with that table and derives the descriptor using its table index. Those descriptors are `FFC704` and `FFC876`, both ordinal zero. Pointer, stageword and ordinal must agree before the ordinary verified-victory contract can emit the password goal.

### King Tyranno

On the public [achievement page](https://retroachievements.org/achievement/205017), author Tybis describes the evolution as defeating four carnivores without dying. A later player report corroborates it and identifies the purple carnivore as Tyranno.

The ROM independently matches that behavior. `C74C..C778` selects the winning actor with species low nibble 3. Opponent species low nibble 3 or 4 enters `C798`, which increments that actor's byte at A2+F. Exactly four selects `C7A6`, writing the King marker `4B` at A2+E, followed by the evolution event trap and health adjustment.

The observer requires a canonical player actor pointer: `FF5F44..FF6724` on a 16-byte boundary. The actor must be an adult species 3 with counter 4 and no existing King marker at `C7A6`. At `C7B8`, the same live A2 must have marker `4B` and counter 4. Only a numbered descriptor below 80 qualifies. Enemy actors, secret stages, an already evolved King and a four-kill counter without the transition do not qualify. This goal does not require winning the battle.

### Surfing Rex

On the public [achievement page](https://retroachievements.org/achievement/193205), author Tybis advises: “Try using Tsunami with no units in the way.”

The player Tsunami code counts the two factions at `1222C` and `12238`. If both counts are zero, `12244` calls `1429E`; the occupied branch calls `14070`. The empty-unit animation calls surfer sprite animator `14566` at `14446` and `14466`.

Qualification requires successful player cast `12224`, empty-unit animation entry `1429E`, and actual surfer animation `14566`, in that order in an armed eligible battle. The occupied animation clears the candidate. It emits once per battle and does not require victory.

## Verification and limits

The standalone C11 observer test compiles with `-Wall -Wextra -Werror` and prints `DB1 local achievement contracts passed (76 goals)`.

Coverage includes every numbered descriptor and final stage, Hard's two awards, password indices, duplicate finishes, missing battle entry/return, loss, malformed or mutated descriptors, campaign disagreement, eligibility cancellation and short RAM. Special tests cover Rain below/above threshold and failed casts; each missing disaster and unsuccessful cast branches; enemy counter increments versus jumps, pre-existing values and player removals; timeout's zero/pre-existing flag cases; populated versus empty surf animation; and King actor ownership, alignment, kill count, prior marker, changed pointer, secret stage and eligibility cancellation.

These are synthetic event-contract tests, not an assertion that all 76 goals have been earned through full end-to-end gameplay. The implementation intentionally requires observed game events and may be stricter than unavailable service-side trigger definitions. A future ROM revision needs fresh address and semantic validation before enabling eligibility.
