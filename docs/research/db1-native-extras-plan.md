# DB1 native Extras and level selection plan

Scope: DB1 only, after the immutable 0.1.6 candidate. Research reads the original
Japanese image (SHA256 b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c)
and current host modules. No existing code or build output was changed for this
plan. Native stage access must use controller input and the original Password
parser/loader; no PC jump, RAM menu index4, stageword or descriptor mutation.

## Original title menu contract

| PC/state | Original behavior / proposed observation |
| --- | --- |
| 1C9CE | Builds title, then initial PRESS START sequence; not a mature menu gate. |
| 1CAA0..1CAD2 | Leaves PRESS START, builds menu sprites, enters1CB30. |
| 1CB30 | Draws selection via1CE16. Entry candidate; repeated after navigation. |
| 1CB34..1CB44 | Native release gate calls2B38 until both action D0 and direction D1 are zero. |
| 1CB46 | Release gate completed; mature menu can be latched here without guessing from artwork. |
| 1CB4A /1CB4E /1CB54 | Main loop sync, original2B38 poll, then action/direction dispatch. Require observed entry before treating these as mature. |
| FF00FE | Word native selection0..3 only: New Game, Continue, Options, Password. |
| 1CB7A..1CB9A | Up:0 wraps to3, otherwise decrements; clears native idle counter. |
| 1CB9C..1CBC0 | Down:3 wraps to0, otherwise increments; clears idle counter. |
| 1CBC4 | Nonzero native action dispatch; leave host title ownership immediately. |
| 1CC16 | New Game: stageword=20*RAM[003C], descriptor=FFADDE+stageword*4A, then original1950. |
| 1CC5C | Continue: original1950 using existing native continuation state. |
| 1CC7C /1CC80 | Original Options path. |
| 1CC92 /1CC96 | Original Password path1D7B6; accepted return1CCAC calls1950. |
| 1CBEA | Native idle demo/branding transition; clear host ownership. |

The native branch treats every nonzero selection other than1/2 as Password.
Writing native selection4 would therefore not add Extras. The fifth row must be
entirely host-owned. The idle counter at FF001A reaches0E10 guest frames before
native demo transition. A visible title extension must not pause that behavior;
an open Extras modal may use the existing host modal pause. Never reset this
counter directly. Abort pending automation on native demo/scene changes.

## Exact native artwork and fifth-row placement

ROM table1D4F2..1D5BD describes34 native sprites, six bytes per source entry
(Y,pattern/attributes,X);1D01C expands them into the live SAT. Four cursor
sprites are first, then native text. Live SAT is at(reg5&7F)<<9, normally F000.

- New Game: glyph origins x128,136,144,160,168,176,184; y152.
- Continue: x128..184 in8 px steps; y164.
- Options: x132..180; y176.
- Password: x128..184; y188.
- Proposed EXTRAS: x136,144,152,160,168,176; y200 (visual center near204).
- Native alphabet: A=tile30A through Z=323. EXTRAS uses
  30E,321,31D,31B,30A,31C. Decode live VRAM patterns; do not publish copied ROM
  glyph payload. Palette0 is unselected; palette1 (attribute2000) selected.
- Cursor main glyph32C: left x116, right x195 with H flip0800; y151+12*row.
  Shadow glyph32D: x115/194, y152+12*row; palette0.1CE16 writes main palette1.
  Native cursor source positions are table1D708..1D717.
- Cursor opaque bounding boxes: left x115..123, right x194..202, main-row y
  through y+8. Password anchor therefore y187..195; New Game anchor y151..159.
  New Extras cursor uses main y199, shadow y200, otherwise identical geometry.

Render only in the mature, explicitly enhanced title menu. Preserve the first
four native rows. While virtual Extras is selected, remove the two native arrow
pairs at the anchor row, including their shadows, by composing the same native
underlay without those four identified SAT sprites. Do not fill black boxes or
replace the whole menu. Plane B plus any opaque native plane A/other sprites must
remain correct. Then draw the native arrow/glyph patterns at the fifth row.
Use live CRAM and actual flips/transparency so native white/black fades remain
owned by the game. Validate SAT/pattern/scene identities before drawing; fail
closed rather than erase unknown artwork. Current title identification helper
and its plane/sprite sampling can be reused, but this is not yet integrated.

## Pure virtual-menu module

New db1_title_menu state defaults disabled. The enhanced presentation integration
explicitly enables it by default for Enhanced; Original leaves it disabled and
receives exactly the original four choices and unchanged input. The state owns
no CPU, RAM, VDP, SDL or provider pointers.

Proposed API: reset; enable(enhanced); before(pc,native_selection0..3);
frame(abstract_buttons,override,pad) returning a single open-Extras request;
read-only active/selection getters; host_closed to re-arm release without losing
the selected Extras row. Root owns renderer and host opening/closing integration.

Entry1CB30 is only a candidate. Mature activation requires the verified release
exit1CB46 or main-loop hooks with a preceding entry. Original intro, title fades,
options and password are not intercepted. Repeated menu hooks preserve state.
Clear at native dispatch/demo/options/password/stage/branding transitions and
reset. Invalid native selection or unexpected anchor change fails closed.

Virtual selection4 retains native anchor0 or3. Down from native3 or Up from
native0 is consumed and selects Extras without changing native RAM. Up from
Extras targets Password3; Down targets New Game0. If the native anchor already
matches that target, consume the edge and re-arm release. Otherwise send only
the required original direction through the ordinary pad path, wait for native
selection acknowledgement, and suppress competing confirmation. A bounded
3-frame press/3-frame release and physical release guard prevent repeats or
held-button leaks. No unchecked selection arithmetic reaches guest RAM.

Confirm/START on Extras opens the host modal from the frame input boundary, not
inside a68000 instruction callback. It never reaches native dispatch. The modal
owner notifies host_closed on Back; Extras remains selected and held buttons
must be released before reopening. Native first-four confirmation/navigation
otherwise passes through unchanged. Unit tests must cover disabled/intro phases,
entry maturity, both wrap anchors, held input, host return, acknowledgement,
unexpected native changes and lifecycle resets before any integration.

## Level catalog policy, including no-save starts

Options1E4CE/1E520 cycles difficulty0..2 with no progression flag gate. Commit
1E78A writes RAM003C, and New Game1CC16 selects the initial native descriptor.
Therefore Practice1(index0), Normal1(index20), Hard1(index40) are available in a
fresh profile. Their qualified table codes are AAIA/LPRP/ODPA in this image;
read original encoded bytes from the effective ROM, never synthesize ASCII.
Baseline entries are ephemeral access policy, not displayed-code evidence, and
must not be written to the saved-code store or grant achievements.

Root-selected convenience policy: validated saved stage N exposes1..N in the
same Practice/Normal/Hard/Back group. This is a level-picker frontier policy,
not proof earlier stages were displayed, played or cleared. Existing achievement
eligibility remains independent. Back has no baseline: require a validated Back
save (MHRI source28F1A legitimately grants Back1). Completing Hard20 alone does
not grant it unless the displayed continuation is actually saved. Special maps
(host mode4,index80..96, ordinal0) require their exact saved code, without a
frontier. Exclude reserved indices8..19 and invalid/stale-ROM records. Use actual
qualified ROM table bytes for every exposed request, with source_pc0 on ephemeral
requests, not a forged renderer hook.

The new pure db1_level_catalog module is independently assigned. Merge/sort and
deduplicate the baseline/frontier/explicit special entries deterministically.
Corrupt/unavailable storage must not remove the three ordinary native starts;
separate qualified-ROM access from persistence availability and retain unknown
files untouched. No saved code alone is a clear-history or achievement grant.

## Launch through original Password, without guest control-flow changes

Natural title Extras gives a bounded original navigation path. On accepted level
selection, copy the validated code, close the modal and hand guest-pad ownership
to a separate native navigator. The title virtual-menu filter must be suspended
for this exclusive handoff, so automatic arrows cannot select its fifth row.
Observe actual native cursor0..3 and stable menu phase; drive Up/Down to row3,
confirm through the live physical-button remap, then use the already qualified
Password prefill/parser/loader replay. Require acknowledgements and abort on
scene drift, user cancellation, native rejection or bounded timeout. No native
menu variable or PC is writable.

The modal should close automatically after launch acceptance (provider return1)
and restore ordinary input after release. Current provider returns0 and leaves
guidance open; this must change only with a working native navigator. Starting
from title avoids boot's4500+ frames consuming the1800 replay timeout. Handle
native idle expiry during handoff explicitly; real original directional actions
reset idle through1CB7A/1CB9C, not host writes. Outside the verified title menu,
retain clear return-to-title guidance rather than silently aborting a battle.

Runtime SaveTest currently selects Extras row0 and expects first saved code.
After baseline catalog integration it must search by expected mode/stage/code
before confirming. Row0 will now be Practice1. Do not misinterpret that test
assumption as a native replay regression.

## Qualification before shipping

First pure state/catalog contracts; then native title/Back/Options/Password
regression in Enhanced and unchanged Original, held buttons and modal return,
both wrap directions and idle boundary. Then fresh-profile starts0/20/40 and
persisted ordinary/MHRI/special selections through original Password, checking
actual parser acceptance, canonical descriptor and later stage presentation.
Test cancellation/timeout and no latched pad. Capture English/Japanese, native
4:3 and a real1440p widescreen window with composed host overlays. Native code,
statistics and logo/background pixels outside the intended glyph/arrow areas
must remain unchanged, including fades. Prepared fixture routes establish
integration only, not a campaign playthrough. DB2 stays paused.

## Pure module implementation status

`games/db1_title_menu.c/.h` and `tests/db1_title_menu_test.c` now implement the
host-only state described above; no renderer/provider/adapter/CMake integration
was performed by this task. The module defaults disabled and enable is
idempotent, so root may apply its Enhanced/Original getter every frame without
resetting navigation. It exposes mature active/virtual selection getters and a
single deferred Extras-open edge. All guest menu indices remain read-only.

TDD evidence: the initial no-op implementation failed mature-menu activation.
The implementation passed C11 `-Wall -Wextra -Werror` for Original pass-through,
intro exclusion, entry/release phase maturity, both virtual wrap anchors,
confirmation precedence on native rows, modal return and held-button guard,
bounded 3+3 native wrap pulse, native acknowledgement, invalid/changed anchor,
idle/scene/dispatch reset, and missing-ack timeout. An additional long-held-key
assertion failed before restricting timeout to missing acknowledgement only.
After native acknowledgement, physical release may take as long as the user
holds the key; the module keeps pad zero rather than restoring that held key
and accidentally causing a second original navigation step. That negative and
positive case now pass as well. Native integrated fifth-row captures remain
pending root renderer/adapter wiring and are not inferred from pure tests.
