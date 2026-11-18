# DB1 unit parameter contracts

Checked 2026-11-18 against the private original 1,048,576-byte Japanese DB1
ROM, SHA-256 `b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c`.
No DB2 data or translated ROM was used. Addresses below are original ROM offsets.
Evidence is direct Motorola 68000 disassembly (Capstone 5.0.7), corroborated by
the generated DB1 C routines. This is a partial recovery, not a finished unit
encyclopedia or proof of player-visible numeric stats.

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

These numbers are **raw words**, not HP, attack, defense, speed, seconds, power,
or costs. In particular species names have not yet been tied to these five
slots by a verified chooser-to-actor trace. Table order alone is insufficient.

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
* `BB38` loads word +4 before calling the subtraction routine. Its triggering
  action still needs a complete call-path/animation trace before naming it a
  reproduction or other action cost.
* `D346` adds word +E to a faction counter; its display units and update
  cadence are not established. It is not a verified power-per-second stat.

The table reader is deliberately neutral: it returns all 64 big-endian words
with slot/address provenance, never infers labels, never runs the CPU or RNG,
never changes the ROM, and preserves output on invalid input or truncation.
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
or A6+36B4 (`AEE2..AF0C`). Their meaning remains unverified.

## Verification

Standalone test (no CMake/shared code changes required):

```powershell
$env:PATH='C:/msys64/ucrt64/bin;'+$env:PATH
& C:/msys64/ucrt64/bin/gcc.exe -std=c11 -Wall -Wextra -Werror -Igames games/db1_unit_stats.c tests/db1_unit_stats_test.c -o build/db1_unit_stats_test.exe
& ./build/db1_unit_stats_test.exe
& ./build/db1_unit_stats_test.exe 'D:/01Claude/Dynabrothers/Dyna Brothers dumped/Dyna Brothers (Japan).md'
```

The synthetic tests exercise endian order, every word/slot, exact-end bounds,
truncation, invalid slots, null arguments, and failure output preservation.
The optional private-ROM test verifies the first and initialization words for
all five slots; the original ROM is neither copied nor bundled.

## Remaining work before guide publication

Trace the chooser labels to slots; name the shared runtime resource using
official instructions plus observed display/behavior; recover attack resolution
and movement timing with terrain/activity modifiers; identify scenario/difficulty
writers; verify egg/upgrade economy and incubation. Until then, retain the guide's
existing qualitative roles and explicit numeric-stat uncertainty. No new
human-facing numeric table is justified by this partial recovery.
