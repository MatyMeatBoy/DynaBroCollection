# DB1 native lightning diagnostic

`games/db1/scenes/lightning-diagnostic.input` enters the first battle through
the normal menus, gives the isolated player a diagnostic power balance, opens
the skull/disasters command and confirms its first lightning entry. It never
changes PC, ROM, dispatch pointers, weather masks or spell state.

## Input and resource contract

- The observed default button map at `FF0028..FF002B` is `03 02 01 04`:
  physical C confirms, B cancels, A opens the overview. `$2B38` reads player 1;
  `$21BE..$23CE` dispatches its mapped action.
- The initial HUD command cursor is `(2,0)`, the egg command. Two separate Left
  presses and Down select `(0,1)`, index 4 in `$23D6`, entering `$4642`.
- The first disaster submenu entry is `$4682[0] = $120F4`. Confirmation at
  `$4696` calls `$55B2`, then `$120F4`, then the native presentation `$132D8`.
- The diagnostic writes packed BCD `00999999` to `FF7BD6..FF7BD9`, the player
  power field at `FF252C+$56AA`. The first disaster's cost at ROM `$11DE4` is
  packed BCD `00001000`; `$128D4/$12914` check and debit the original balance.
  This is a resource fixture, not earned campaign progress.

## Three separate presentation contracts

1. `$6372/$65F2` cull persistent weather to the original six-by-six display.
   The authoritative coordinate lists are `FF181A` and `FF1864`, each at most
   37 `(worldX,worldY)` pairs with an X=`FF` terminator. Bits 2/3 of `FF0168`
   enable them. ROM `$64F4` supplies a 4×4 tile metatile; bit 6 selects `$6514`.
   The host extends these exact lists and current VRAM patterns without RNG.
2. Generic spell sprites use `$5FF6..$62FC`; their art can reuse HUD tile
   numbers. A presentation-only bounded SAT walk preserves their complete
   geometry beyond native column 320, linked first-wins order and plane
   priority. It does not evaluate VDP status or change the SAT.
3. The user's lightning entry is `$132D8..$13630`. It authors clouds and bolts
   across plane A at `C000`, changes Hscroll to cell-row mode, and temporarily
   replaces the normal HUD. It must be sampled as one authored plane across
   the full output width. Treating columns 208..319 as a HUD creates the exact
   missing-cloud strip and displaced bolts in the reported symptom.

`$5CCC/$61DA` can also displace plane B independently of camera RAM. Expanded
terrain and host actor coordinates use actual B scroll minus `FF0200/FF0202`
so the original/expanded seam shares one shake phase.

## Verification

The initial native run, isolated copied executable SHA-256
`5a77a036100f83ee8f1d1211718750977735ca598807a9b82f070fc1cbfcb2ba`, completed
8000 frames without an AOT miss. Its `lightning-menu.png` displays the bolt
icon; `lightning-120.png` reproduces the cloud gap at x=208..285 and displaced
right-hand lightning. Private captures are under
`local/evidence/db1-lightning-agent-a/` and are not shipped ROM assets.

The pure video contract test includes positive and negative tests for the
authored lightning plane, persistent weather cells, alternate weather art,
SAT geometry past column 320, HUD priority, spell/HUD art reuse, camera shake,
duplicate actor suppression, and immutable RAM/VDP inputs. Integrated native
captures must be rerun after dispatching both `$132D8/$13630` observer hooks.

### Integrated result, 2026-11-18

The relinked isolated executable SHA-256
`552d48daa905965fe53096aa197f2e1c13ef9e0cfa688b8a763e9eb1f22104b2`
completed the same 8000-frame diagnostic. The inspected corrected capture is
`local/evidence/db1-lightning-agent-fixed/lightning-120.png`: its cloud band
is continuous across all 398 columns and its bolts are no longer displaced
or cut at the former HUD seam.

- The old and corrected expanded renderers differ at **zero pixels** in
  x=0..207 across all 224 rows of that capture. The former cloud-gap rectangle
  x=208..285, y=0..63 changes at 3690 pixels; the right region changes at 4944.
- Both runs finish with canonical state FNV `8DD2CC208312524F` and audio-state
  FNV `17EEF8B377D8E8CF`. This fix does not change their simulation outcome.
- A third 8000-frame run with `--widescreen off` also passes and ends with
  those same state/audio digests. Its 320×224 screenshot matches the corrected
  renderer's first 320 columns everywhere except 135 creature pixels in
  bounding box `[145,79)..[174,98)`. Those pre-existing host actor animation
  differences are separate from the lightning fix: all cloud and bolt pixels
  match the native output. Do not claim complete native ARGB identity.

The power balance remains a labeled diagnostic fixture. These captures qualify
the native lightning command and the reported visual seam, not campaign play.
