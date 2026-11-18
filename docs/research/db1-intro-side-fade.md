# DB1 mural/story side fade

## Native phase and baseline evidence

The original routine at `1C5A4` contains both publisher logos and the subsequent
mural/story. A single `BRANDING` scene flag therefore cannot safely enable this
effect: the SEGA/CRI backgrounds must continue their native black/gray/white
palette transitions without spatial dimming.

The generated native instruction listing in
`generated/db1-en/db1_part09.c` identifies the narrower phase:

- `1C7D0` selects story palette source `1D340`, after the logo animation loops.
- `1C80E` and `1C858` decompress the two mural resources.
- `1C894` waits for VBlank; `1C89A` and `1C8EE` call the story update at `1CEC8`.
- `1C9CE` starts title initialization. Existing title re-entry hooks `1CAF6` and
  `1CB0C`, intro restart `1C5A4`, and gameplay dispatch `1950` clear the flag.

Actual unmodified native controller-flow captures before this effect are in
`local/evidence/db1-intro-side-fade-before2/`: SEGA gray fade at wall frame241,
CRI white at361, black at481, mural fade at601, mural at721, lower story text
at1201, and title after Start. The source binary is the earlier white-margin
fix, SHA256 `7c86ea95dc741a94b124ef691e8144420cfe24085c46c410ae87e84291b8e1ed`.
The fixture is `games/db1/scenes/intro-side-fade.input`. These captures establish
the existing abrupt mural/text side edges, independently of the new effect.

## Rendering contract

`db1_intro_fade.c` owns only a presentation-phase flag and a pure row operation.
It never reads or writes guest RAM, CPU, VDP state, scroll tables, or assets.
The integration must forward the sparse phase PCs, reset with the video
provider, and call the row function after final enhanced row composition.

The explicit `story` argument permits use of a captured frame flag. The explicit
`enhanced` argument and `width > 320` guard preserve original 4:3 presentation.
Only the outer sixteen pixels on each side of the centered native 320px image
can change. The central 288 pixels and all added margin pixels remain exact.
All native rows 0 through223 use the same operation, including the lower text
region; there is no scene-content classifier or vertical cutoff.

A cubic smoothstep changes RGB contribution from exactly zero at the outside
pixel to exactly one at the sixteenth pixel. It blends toward the caller's live
authored backdrop, using integer arithmetic with rounded channels. It samples
no neighboring pixels and introduces no scaled blur. Original alpha is retained.
SEGA, CRI, title, gameplay, invalid geometry, and disabled/original presentation
are no-ops.

## Verification

The standalone test first failed with a no-op implementation at native story
activation. The implemented module passes compilation with `-Wall -Wextra
-Werror` and covers phase lifetime; all224 rows; multiple widths; exact side
endpoints; monotonic symmetric ramps; unchanged central/margin pixels; colored
backdrops; null input; original4:3; disabled enhancement; nonstory phases; and
invalid row/width geometry.

Set `DYNA_INTRO_FADE_TRACE=1` for sparse runtime evidence. It reports transitions
only, with the actual native PC and resulting story flag. Final integrated
native captures and original-mode comparison are required before presenting the
effect as visually verified; unit coverage alone does not establish this.

## Integrated native qualification

The integrated private executable and assets were copied from `build/dev` before
probing; no relink was performed by this qualification. Executable SHA256:
`2c8257bcbcbd0c47cb3b618aa629828f5210eaa3ba7a4ec3645ec7842e31f52c`.
The identical2500-frame fixture completed in enhanced fit and original off modes.
Evidence folders are `local/evidence/db1-intro-side-fade-after/` and
`local/evidence/db1-intro-side-fade-original/`.

Runtime phase trace confirms native `pc=01C7D0 story=1`, followed by
`pc=01C9CE story=0`. All three full logo captures are byte-identical to the
baseline, including SEGA's gray fade, CRI white, and the black transition.
All seven original-mode320x224 captures are byte-identical to the baseline's
native center, including the title.

For mural fade, mural, and lower text captures, every pixel in all224 rows was
checked against the exact sixteen-pixel smoothstep blend toward the native
black backdrop. Center288 pixels and external margins are byte-identical.
Changed pixel counts are896,4383, and6299 respectively. The text capture has
220 visibly changed rows, including1916 changed pixels below row151; the four
remaining rows were already black. The central character and readable text
remain pixel-identical outside the prescribed side bands. The actual resulting
story capture was visually inspected.

Before, enhanced after, and original off share canonical guest state hash
`621F9F4A775D6389` and audio state hash `759F1040B4770EA0`.
Detailed pixel results are in `pixel-qualification.json` alongside after captures.
This verifies the exercised intro sequence; it does not assert unrelated scenes
or device-specific presentation behavior.
