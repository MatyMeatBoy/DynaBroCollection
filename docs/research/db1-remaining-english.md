# DB1 remaining English audit (2026-11-18)

Read-only review of `tools/translate_db1.py`, `tools/generate_language.py`,
`translations/db1-en.json`, existing research and prepared local images. This
inventory separates verified coverage from leads. No new Japanese phrase was
safely qualified for translation in this bounded pass.

## Reproduced coverage

`python tools/analysis/audit_db1_translation.py local/roms/db1-en-v01.md local/roms/db1-en-950A8FC3.md`
reconstructed the current image in memory from the guarded catalog and obtained
byte equality. Source CRC32 is C1E5C0DA; result CRC32 is 950A8FC3 and SHA256 is
ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261.
Neither image was written. The new audit script emits identities and offsets,
never cartridge byte payloads.

| Area | ROM offset / codec | Native route | Coverage |
| --- | --- | --- | --- |
| Six pause-map labels | 1B464, 108 bytes, BE word tile indices; font offsets 1B510, 40 bytes | map-labels.input | Current catalog; already qualified |
| Eighteen credits names | 2950A, 180 bytes, 18 x 10 tile bytes | credits-diagnostic.input; selector 2870A, writer 29018 | Current catalog; already qualified |
| Hard-mode secret | 29CE4, 234 bytes, 9 x 26 tile bytes; A=0C | credits-diagnostic.input, stage 3B | Current catalog; already qualified |
| Practice completion | 478E, 100 bytes; A=A9, NUL row / double-NUL end | practice-completion-diagnostic.input, original entry 48AE / renderer 4BEA | Current catalog; already qualified |
| Alien completion | 481E, 101 bytes; same body codec | alien-completion-diagnostic.input, original entry 48B6 | Current catalog; already qualified |
| Completion backdrops | 477C and 480C, 16 bytes each; A=14, blank=FF | Same respective completion routes | Current catalog; already qualified |

`generate_language.py` packages differences into reversible spans across two to
four equal-size images. It does not discover or translate missing strings. A
new catalog edit requires fresh generated language data; simply changing an
image on disk cannot establish that the running native binary contains it.

## Open inventory and next concrete work

| Item | Evidence / natural English | Implementation needed |
| --- | --- | --- |
| Title | The older credits inventory is stale here. `db1-title-art.md` documents an approved English logo, implemented by `games/db1_title_art.c` with `assets/img/dyna-logo-v1.png`. Existing native captures cover English/Japanese and fades. English wording is the supplied logo. | No new translation justified. Confirm current packaged assets include the logo. This is a native compositor treatment, not a ROM text patch. Plane A tile envelope 102..262, VRAM C000; guards 2102 at column 13 row 3 and 2262 at column 31 row 17. |
| P-chan speech bubble | Existing credits inventory identifies this as unresolved, but provides no screenshot, transcription, offset or codec. No phrase was independently recovered in this pass. | Capture original Japanese bubble with guest PC/VRAM/SAT state; identify whether sprite or plane tiles; trace upload/decompression to ROM resource. Obtain exact Japanese before proposing English. Do not guess a phrase from the character's egg-eating role. |
| Other secrets / ending branches | No complete reachable-scene list exists in inspected evidence. The prepared completion and credits routes do not prove normal campaign completion. | Enumerate original result/ending selectors and all referenced resources; capture each distinct branch in Japanese and current English before translating. |
| Staff credits | Credits research explicitly says these use a separate table and were preserved. No evidence here establishes all lines' language or need for replacement. | Inspect the separate table and full scroll capture; preserve personal attributions and translate only actual untranslated roles after decoding. |
| Intro/options/password/defeat/ordinary victory | Earlier bounded routes establish some existing English; the intro contact sheet directly shows English narrative. No exhaustive all-stage or all-option proof. | Extend scene matrix with actual captures; inventory any discovered text by resource and renderer. |

## Rejected shortcuts

Byte-pattern searches for CP932 half-width katakana in unchanged original/current
ranges mostly return graphics/music-like repetitions and ascending tile maps.
For example 2ABD0 onwards contains alternating attribute/index sequences, not
qualified prose. Credits names instead use custom tile codes, so a generic text
codec scan both misses real text and invents false positives. No offsets from
these scans qualify as translation candidates without a renderer/resource trace.

The current title source hooks are `games/db1_video.c` calls to
`db1_title_art_row`; a cartridge-only English image will still contain the
Japanese title graphics. Full-English native presentation and full-English ROM
patch are therefore distinct deliverables that need separate qualification.
