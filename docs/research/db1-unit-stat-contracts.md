# DB1 unit parameter contracts

Checked 2026-11-18 against the private original 1,048,576-byte Japanese DB1
ROM, SHA-256 `b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c`.
No DB2 data or translated ROM was used. Addresses below are original ROM offsets.
Evidence is direct Motorola 68000 disassembly (Capstone 5.0.7), corroborated by
the generated DB1 C routines. The recovered vitality, reproduction and combat
contracts below are suitable for a guide with their explicit units and limits;
they do not establish a complete encyclopedia, movement speed or player costs.

## Located table

Several routines mask actor type with `0x0f`, shift left seven bits, and index
base `0xF08A`: `CA8C..CA9C`, `CC42..CC52`, `D62A..D638`, `D88E..D898`.
Slots 1 through 5 contain 128-byte parameter entries. Slot zero begins with
unrelated executable bytes and must not be decoded as a dinosaur entry. The
next table begins at `0xF38A`; its fields are not part of this decoder.

| Slot | Entry offset | Word +0 | Word +2 | Word +4 | Word +6 | Word +8 | Word +A | Word +C | Word +E |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | F10A | 800 | 1800 | 800 | 1000 | 200 | 1300 | 200 | 8 |
| 2 | F18A | 800 | 2600 | 900 | 1000 | 200 | 1300 | 200 | 9 |
| 3 | F20A | 700 | 2000 | 1000 | 1500 | 0 | 0 | 200 | 10 |
| 4 | F28A | 1000 | 3000 | 1000 | 2000 | 0 | 0 | 200 | 10 |
| 5 | F30A | 800 | 2500 | 1000 | 1500 | 400 | 1400 | 200 | 20 |

Words +8/+A/+C/+E remain incompletely named and must not be presented as attack,
power, speed or costs. The first four words now have qualified semantics below.

## Verified species mapping

The original credits bind names to the same graphics resolver used by actors:
`2870A` sets the name cursor to `2950A`, and `28704` sets the actor-sequence
cursor to `293BE`. `288BE..288D8` consumes ten name tiles per character;
`28960..28966` consumes type/animation/direction triples and calls `28180`.
Four successive triples describe each character, advanced by the animation
cycle at `28816..2883E` before the next name. The first five groups select
adult types `31`, `32`, `33`, `34`, `35`, respectively. `28180..281A6` resolves
these through the actor graphics table at `70010`, using the same type-minus-one
layout as the normal renderer. These are not ordinal guesses from the parameter
table. The original Japanese name rows were independently decoded in
[the credits research](db1-credits-translation.md): Stego, Tricera, Tyranno,
Allo and P-chan. The Tyranno species-3 identification also agrees with the
separately traced King evolution routine at `C74C..C7C4`.

## Qualified guide statistics

Use **vitality** as an editorial English name for the shared internal resource:
combat, reproduction and activity all change it. It is not a separate food bar
plus HP, and none of these numbers is a player power price. No exact native
Japanese label or visible HUD number is claimed for this field.

| Dinosaur (slot) | Initial vitality | Low-vitality threshold | Reproduction requires | Parent vitality spent per egg |
|---|---:|---:|---:|---:|
| Stego (1) | 1000 | 800 | greater than 1800 | 800 |
| Tricera (2) | 1000 | 800 | greater than 2600 | 900 |
| Tyranno (3) | 1500 | 700 | greater than 2000 | 1000 |
| Allo (4) | 2000 | 1000 | greater than 3000 | 1000 |
| P-chan (5) | 1500 | 800 | greater than 2500 | 1000 |

The reproduction threshold is **strictly greater**, a necessary condition,
not a guarantee that an egg appears. Space, terrain, actor availability,
current behavior and the call path must also permit reproduction. Initial
vitality is an initialization constant, **not maximum or current vitality**.

The following matrix is vitality subtracted from the opponent **per execution
of the ordinary combat-drain call at C730..C734**, not DPS, win probability,
or a universal species attack rating. Columns identify the opponent's low-nibble
species slot. Ordinary dinosaur-vs-dinosaur matching uses the named columns;
other creature families sharing those slots use the same selected column.

| Attacker / target | Stego | Tricera | Tyranno | Allo | P-chan |
|---|---:|---:|---:|---:|---:|
| Stego | 90 | 70 | 60 | 40 | 95 |
| Tricera | 450 | 350 | 350 | 250 | 475 |
| Tyranno | 900 | 700 | 480 | 320 | 960 |
| Allo | 1350 | 1050 | 320 | 480 | 1437 |
| P-chan | 36 | 28 | 24 | 16 | 38 |

This identifies why a single attack-number label would be misleading: Tyranno
and Allo have different opponent-dependent rows. It also establishes P-chan's
small combat drain without contradicting the official description that its
offensive role is eating eggs rather than directly hunting enemies.

## Recovered uses and limits

* `AF06`, `C576`, `DBD0` copy word +6 into secondary actor record word +2.
  `AF06` selects the slot by masking the type then shifting by one and six,
  equivalent to the seven-bit shift above.
* `D5EC..D5FE` subtract D5 from that runtime word and clamp at zero.
  `D60E..D61E` add D5 and clamp unsigned overflow at 65535.
* `D622..D656` compare the runtime word against table word +0, setting
  primary actor byte +A to one when runtime word is at most that threshold,
  otherwise clearing it. Thus word +0 is a threshold, **not a maximum**.
* `AD56` checks whether the same runtime word is zero and branches into
  condition/depletion handling (`CF42` or `CF76`, depending on terrain query).
  `D5AA` drains it according to activity and terrain, using word arrays at
  table +20 and +40. This prevents calling it combat-only HP without further
  work. No frame-to-time conversion is established.
* `CAAA/CAB2` select words +8/+A using the XOR of actor IDs' high bits, then
  call the **addition** routine D60E. `CC60/CC68` use word +C on both branches
  and also add it. These values must not be advertised as attack damage.
* `BAD0..BAEA` selects the parent's species entry and rejects vitality at or
  below word +2 (`BLS`). `BB02..BB24` searches adjacent terrain/occupancy;
  `BB2C` obtains a free actor via `D218`, and `BB34` invokes `C40E`.
  `C412..C428` copies the species low nibble and adds egg family `10` or `20`
  according to faction. `C42C..C486` initializes this new egg and its resource,
  and `C490` stores the parent actor ID in its secondary record. `BB38..BB3C`
  then subtract word +4 from the parent. This proves reproduction threshold
  and parental vitality cost; it is not the player's egg-creation power cost.
* `C71A..C730` selects attacker low-nibble species row and target low-nibble
  species column: `base + attacker*128 + 0x14 + (target-1)*2`. `C734` invokes
  `D600`, which redirects the saturating subtraction to the opponent's record.
  `C73C..C744` repeats that same drain if the attacker's evolution marker at
  primary record +E is nonzero and the first drain did not already deplete the
  opponent. Consequently the ordinary matrix must not be presented as the
  complete evolved King attack amount. The fourth carnivore victory sets the
  King marker and resets its vitality to table +6 (`C798..C7C4`).
* `D346` adds word +E to a faction counter; its display units and update
  cadence are not established. It is not a verified power-per-second stat.

The raw reader returns all 64 big-endian words with slot/address provenance.
`db1_unit_stats_read` additionally exposes only the proven vitality,
reproduction and combat-drain fields. Neither reader runs the CPU or RNG,
changes the ROM, or changes output on invalid input or truncation.
It validates bounds, not ROM identity. Callers must independently identify the
ROM before treating decoded words as the original DB1 data.

## Difficulty, life stage, faction, and runtime state

The observed table indexing uses the low nibble of actor type; higher type
bits and faction ID are not additional table dimensions at these call sites.
This does **not** prove difficulty independence: tracing all parameter writers,
runtime modifiers, maturation, terrain, combat and scenario setup is unfinished.
The initialization value must never be represented as an actor's current value
or maximum. Both factions use the same table at the shown sites, while runtime
record word +4 is separately initialized from faction-specific arrays at A6+36A4
or A6+36B4 (`AEE2..AF0C`). `DBFA..DC46` obtains the family/type choices from
stage descriptor bytes +E..+17, and `DC92..DD72` maps types 31..65 to 0x100-
spaced entries in the subsequent animation/behavior table. `D098` and `D120`
use that runtime selector with direction and animation state to load action
countdowns. Thus creature family and stage setup affect action timing even
where the shared low-nibble vitality/attack table is unchanged. A universal
species movement speed or attack-per-second number is not established.

## Verification

Standalone test (no CMake/shared code changes required):

```powershell
$env:PATH='C:/msys64/ucrt64/bin;'+$env:PATH
& C:/msys64/ucrt64/bin/gcc.exe -std=c11 -Wall -Wextra -Werror -Igames games/db1_unit_stats.c tests/db1_unit_stats_test.c -o build/db1_unit_stats_test.exe
& ./build/db1_unit_stats_test.exe
& ./build/db1_unit_stats_test.exe 'D:/01Claude/Dynabrothers1+2/Dynabrothers/Dyna Brothers dumped/Dyna Brothers (Japan).md'
```

The synthetic tests exercise endian order, every word/slot, exact-end bounds,
truncation, invalid slots, null arguments, and failure output preservation.
The optional private-ROM test verifies all four named vitality/reproduction
fields and every cell of the combat matrix for all five species. Synthetic
tests also verify the typed reader's field mapping and failure preservation.
The original ROM is neither copied nor bundled. Both test modes passed with
GCC `-std=c11 -Wall -Wextra -Werror` on 2026-11-18.

## Official abilities and concise guide wording

The [official Sega character profiles](https://vc.sega.jp/vc_dyna/) and
[instructions](https://vc.sega.jp/vc_dyna/instruction.html), read 2026-11-18,
support the following original English summaries. Numeric sentences come from
the contracts above, not the website:

* **Stego:** Your main breeder and power supplier. Initial vitality 1000;
  reproduction needs more than 1800 and spends 800 vitality. Protect it while
  expanding grass.
* **Tricera:** A defensive herbivore that fights back, but breeds less readily.
  Initial vitality 1000; reproduction needs more than 2600 and spends 900.
  Its combat drain is 450 against a Stego-slot opponent, compared with Stego's
  90 against the same slot; this is one matchup, not a universal fivefold rating.
* **Tyranno:** The attacking carnivore that heads toward the enemy. Initial
  vitality 1500; reproduction needs more than 2000 and spends 1000. Its ordinary
  combat drain against Allo is 320. Official instructions limit its walking
  route to grass and wasteland, so prepare a route with rain.
* **Allo:** The local interceptor and egg defender. Initial vitality 2000;
  reproduction needs more than 3000 and spends 1000. Its ordinary combat drain
  against Tyranno is 320. Official instructions warn that without arriving
  enemies it can become hungry and eat friendly dinosaurs.
* **P-chan:** A fast egg raider that disrupts enemy breeding and power growth.
  Initial vitality 1500; reproduction needs more than 2500 and spends 1000.
  Its ordinary combat drain ranges from 16 to 38 across the five target slots,
  making egg raids the useful role; no movement-speed number is established.

Overall guide note: "Vitality is the shared internal reserve used by combat,
activity and reproduction. Starting values are not maximum HP. Birth costs
spend the parent's vitality; they are not egg-creation power prices. Combat
drain varies by opponent and is per attack-routine execution, not per second."

## Remaining qualification and a bounded runtime probe

Movement timing, exact incubation, player egg/upgrade prices, native HUD units,
and all difficulty modifiers remain open. A future probe should use an isolated
existing executable and original-ROM identity, record the scenario descriptor,
species/type, both actor records, table entry, guest PC and frame at AF06/C576,
BAD0/BB34/BB3C and C730/C734; compare the exact before/after record +2 values.
Do not edit actor records to manufacture a result. Reproduction can be reached
with ordinary food growth, and combat with ordinary opposing actors; record
setup separately if a diagnostic power fixture is required. Compare the same
species in normal/hard and at least two family selectors before claiming
timing independence. Check the King transition separately because it resets
vitality and can repeat combat drain. No such new runtime probe was executed
in this pass; the numeric contracts here are static instruction/table evidence.
