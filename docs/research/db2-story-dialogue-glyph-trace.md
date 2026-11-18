# DB2 story dialogue: glyph-trace decoding method (bounded investigation)

Investigation on 2026-11-18, using the real Japanese ROM, the prebuilt native
binary and `DYNA_TRACE_GLYPH=1`. This is a method note and partial, uncertain
decoding — no translation is proposed or added from this pass.

## What was confirmed reachable

Main menu → Story Mode (no cursor move needed; START, START from the title
reaches the mode select with Story already highlighted; a third START enters
Story Mode directly) renders a real intro caption through the same renderer
call site already used for the 170 already-translated strings (PC `0x5B174`,
traced by `games/step_adapter.c`). With `DYNA_TRACE_GLYPH=1` the trace prints
`a1` (ROM glyph-ID stream address) and `word` (the glyph ID at that address)
for every cell drawn.

For the intro caption ("宇宙人軍団が退却してから / 何年もの月日が流れた…",
read directly off the screenshot), the trace shows real, non-zero glyph IDs
starting at ROM `0x25CD94` (matches the `0x25CD94+` dialogue pool already
noted in `local/db2-english-inventory/INVENTORY.md`):

```
25CD94 0B4F   25CD96 10EC   25CD98 0A54   25CD9A 013F   25CD9C 0141
25CD9E 0148   25CDA0 014E   25CDA4 0BEF   25CDA6 0124   25CDA8 0162
25CDB4 0A8A   25CDB6 0ED7   25CDB8 0172   25CDBA 087E   25CDBC 0128
25CDBE 0168   25CDC0 0126   25CDC2 0148   25CDC4 0137   25CDC6 0146
25CDC8 0124   25CDCA 013F   25CDCC 0005   25CDCE 0005   25CDD0 0005
```

This establishes that story-dialogue text **is** driven through the same
generic address-matched glyph_text mechanism as the already-translated
strings (`games/db2_glyph_text.c`, table from
`tools/generate_db2_glyph_text.py`) — no new rendering code is needed to
translate it, only new catalog entries with correct `before` bytes and
English `lines`.

**Not done / not safe to act on yet:** matching each glyph ID to the exact
on-screen character (to build a reusable ID→kana/kanji table) requires
careful, verified correlation across many more captures; a naive word-by-word
guess from this single pass already has a gap (the trace skips `25CDA2`
entirely, meaning the screen text does not line up 1:1 with the glyph IDs in
the simple way assumed above) and un-double-checked segmentation. Treating
this partial read as a lookup table and patching other strings from it would
risk shipping **wrong** characters, which is worse than leaving the text
Japanese. No glyph→character table is committed from this investigation.

## Stage briefing screens are a different, harder case

The Stage 1 briefing ("ステゴひろば / あめをふらせて...") reachable right
after the intro caption (Story → A through the dialogue → Stage 1) does
**not** go through this glyph-ID mechanism at all: the trace shows zero real
`[glyph]` calls (`word=0000`) for that screen's text region. This confirms
the existing finding in `INVENTORY.md` that stage-list/kanji level names are
baked graphics, and extends it: stage *briefing* text is graphics too, not
just the 17-name selection list. Translating it needs tile/graphics
replacement, not a catalog entry — a materially different and harder problem
than the glyph_text mechanism covers.

## Recommended next steps (not started here)

1. Build a verified glyph-ID → character table by capturing many short,
   independently-readable strings (confirm dialogs, short exclamations
   already in the catalog have *known* English meaning and *known* `before`
   bytes — cross-reference those bytes against their original Japanese
   reading in `INVENTORY.md` to seed the table with zero guessing) before
   trusting it on new, unread strings.
2. Once a table exists, transcribe a few short dialogue lines, cross-check
   them independently (e.g. against this screenshot-reading method) before
   writing any new `glyph_text` catalog entry.
3. Stage-briefing/graphic text needs a tile-replacement investigation
   separate from the glyph_text catalog; out of scope for that mechanism.

No ROM, catalog, or generated file was modified by this investigation.
