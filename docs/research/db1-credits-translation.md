# DB1 credits and secret-mode text

OptiPort pass resumed against upstream `58843a2c9122720f5db8649c0d9cfdb60dc479cf` on 2026-11-17. This is development after the closed v0.1.1 checkpoint; its archive is unchanged.

## Recovered contract

The supplied English patch changes the loaded font, leaving Japanese indices in the character-name table at `$2950A..$295BD` (18 rows of ten tiles). The original credits code selects this table at `$2870A` and writes ten bytes per name through `$29018`. Staff credits use a separate table and remain unchanged.

The hard-ending message at `$29CE4..$29DCD` contains nine rows of 26 tiles. The original routine selects it after testing stage `$3B`; it congratulates the player on clearing hard mode, explains playing as aliens, mentions staff-made bonus stages and gives password **MHRI**. Its renderer loads capital A at tile `$0C`, blank at `$00`, period at `$27`, question mark at `$28` and exclamation mark at `$29`. Font graphics were inspected directly; the initial punctuation draft was corrected after a reference screenshot exposed the wrong glyphs.

Both tables now use guarded fixed-width catalog edits. Long display names are shortened: Gumigumi-chan becomes GUMIGUMI, Perorinchicchi becomes PERORIN and Pekipekiman becomes PEKIPEKI. Other names use compact romanizations; these are fan-localized display names, not claimed official English names. Existing staff attributions are preserved.

## Qualified output

- Full image size: 1,048,576 bytes, unchanged.
- English CRC32: `11688440`.
- SHA256: `37891b312529b72cee0ff568f161d8258f528baa04c83f4b0bc6c0ef48de5daa`.
- Relative to v0.2, only the two tables and Mega Drive checksum change. Code and timing are untouched.
- Native language metadata accepts JP, new English, supplied v0.1 and previous v0.2. The old v0.2 is reconstructed from the map-only catalog and checked against its known SHA256 before inclusion.

## Verification and limits

32 Python tests and five native CTest contracts passed. Negative checks reject bad source/table guards, overlapping edits, overflow and unsupported glyphs. Four-variant metadata and native conversion from the fourth variant to both languages are covered. The full BPS roundtrip and unchanged original dump/closed checkpoint hashes were checked locally.

Independent Genesis Plus GX captures show repaired names and the complete secret-mode message. These use private diagnostic copies that redirect the initialized boot jump to the original credits routine and select stage `$3B`; the extra diagnostic trampoline is excluded from the translated image and native language data. They verify rendering, not a campaign playthrough.

Native map regressions passed for original JP with default new English and previous English v0.2 with Japanese selected (8,900 frames each). DB2's shared-language regression passed its 10,010-frame tutorial route after rebuilding for the expanded metadata structure.

Native ending/credits verification uses [an explicit diagnostic input script](../../games/db1/scenes/credits-diagnostic.input), preparing a win and final-stage identity in RAM before the original game handles the result and ending. Its result must be assessed by captures, not only by the finite probe's exit code. The ordinary campaign remains unqualified.

That native route completed 52,000 frames with executable SHA256 `62c0c33e2bd276714438944174c4978d3f674f115f9c1432dcb43c5a3cb7bbc2`. The credits capture shows PURIRIN correctly beneath its character, and the secret-message capture shows all nine English rows including MHRI. Logs report zero interpreter fallback calls. Evidence is retained privately under `local/evidence/db1-credits-stage-native-final`. This binary identifies a development working tree; it is not a newly closed package.

## Remaining DB1 translation inventory

| Area | Current evidence / next work |
| --- | --- |
| Pause maps and surrender | Six repaired labels, reference and native scenes verified |
| Character names in credits | Eighteen fixed-width entries repaired; sampled reference and native captures verified |
| Secret-mode message | Nine rows translated and readable in reference and native captures |
| Intro/options/password/defeat | Existing English observed in earlier bounded routes; no claim of exhaustive coverage |
| Victory results | Garbled output observed during prepared-win diagnostics; qualify the ordinary first-stage context and trace its font/table before patching |
| Title graphic | Still requires graphical-text inventory and an English treatment |
| P-chan speech bubble | Still requires graphic resource identification and translation |
| Other secrets/endings | Need reachable scene inventory and regression routes |

No translation percentage or full-game completion is inferred from these rows. Local ROMs, screenshots and private logs remain outside Git.
