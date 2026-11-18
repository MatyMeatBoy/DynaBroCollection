# Native password progress and Extras level selection

## Scope and evidence

This contract stores **native continuation codes**, not machine snapshots.
Research reads the protected original dumps, never changes them, and uses
Capstone disassembly plus the existing verified achievement descriptor contracts.
Original SHA256 identities:

- DB1: `b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c`.
- DB2 Special VC: `1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76`.

Private disassembly evidence is `local/progress-original-proof.txt`; it contains
no copyrighted ROM payload. The addresses below are proved in those originals.
Presentation translation changes must be separately qualified before reusing
the hooks; persist the effective ROM SHA256, not just the title or filename.

## DB1 displayed result code

`1BEA8` is the shared native four-character password renderer, called by ordinary
winning results (`1BBE8`) and losing results (`1BC08`). It loads ROM table
`1E87E`, reads the signed captured result at `A6-2`, and increments word
`FF0034` at `1BEBA` only when the result is nonnegative. At **`1BECC`**, A1
already points to the exact four encoded bytes that will be displayed and
`FF0034` is already the continuation index. Four bytes are read at `1BEE0`;
each byte is shifted right two and values below 26 map to A–Z. Others render
the placeholder tile `104` and must not be promoted to a usable code.

Recommended observer: at `1BECC`, validate the immutable ROM pointer A1 is
exactly `1E87E + 4*BE16(RAM+34)`, bounds-check all four bytes, require their
decoded indices below 26, copy their **original encoded bytes**, and save once
per displayed code. Code bytes are not ASCII (for example `02` renders A).
Do not derive a continuation from the old descriptor pointer: the win renderer
increments the stageword while the previous battle descriptor can still remain.
Do not save a new stage merely because an outcome routine was entered.

The separate password-entry screen starts at `1D7B6`. Its typed code buffer is
`FF0036..FF0039`; initial `6A6A6A6A` is the blank placeholder, not a password.
Parser `1DCEC` compares all four raw bytes against table `1E87E`, stopping on a
zero first byte. Failure is `1DD1A`; success starts at `1DD28`. At `1DD76` it
writes the matching table index into `FF0034`; at `1DD8A` it writes canonical
descriptor pointer `FFADDE + index*4A` into `FF002E`. **`1DD90`** is an
after-write success hook for a code entered by the player. Require table-match,
index/pointer agreement and the descriptor's ordinal, not merely plausible RAM.

Numbered descriptor groups already independently validated:

| Mode | Table indices | Ordinal |
| --- | --- | --- |
| Practice | 0–7 | 1–8 |
| Normal | 20–39 | 1–20 |
| Hard | 40–59 | 1–20 |
| Back/alien | 60–79 | 1–20 |
| Special password maps | 80–96 | zero, not numbered campaign stages |

Indices 8–19 are reserved and must not be presented as levels. Practice advances
the descriptor pointer without updating the stageword; ordinary battle identity
therefore uses the descriptor. This does not override the result renderer's
explicit continuation-index contract. The table includes independently verified
PPPP at index 87 and PUGU at 92. Use bytes from the actual qualified ROM rather
than recreating those codes from their names.

The hard-ending message also displays **MHRI**, in a different renderer:
the nine-row text at ROM `29CE4..29DCD`, selected after final stage `3B` in the
credits path. This existing verified message is documented in
`db1-credits-translation.md`. It is not reached through `1BEA8`; a claim that
the ordinary result hook covers every password screen would be false. Original
control flow resolves its separate hook: `28E1C..28E26` requires stageword
`3B`; `28EA2` selects the message, calls `29018` for nine rows, and `28F14`
fades it in. **`28F1A`** is the after-fade display hook. Original table entry
60 decodes to MHRI (`32 1E 46 22`), the native continuation into Back stage 1.
The session includes this hook, requiring completed index59 and validating
table60/descriptor ordinal1. Its displayed text was already independently
captured in the existing credits research.

## DB2 displayed result code

Shared result routine `58918` obtains the current ordinal from descriptor
`BE32(FF0278)`. For result argument zero, `58954` increments the ordinal before
calling password renderer **`58E4E`** at `58968`; argument one keeps the current
ordinal. Mode 8 bypasses this password-rendering branch at `5893E..58944`.

Renderer `58EDC` obtains mode from descriptor byte zero and `58EE6` obtains the
target ordinal from word `A6+8`. `58EEA` calls original lookup **`49D60`**.
The condition codes on return distinguish success: `58EF0 BNE 58F38` skips
code drawing on failure. **`58EF2` is the successful display hook**, with A0
pointing into the native ROM password strings. Copy the NUL-terminated code
bounded to six bytes, before A0 is consumed at `58EF8`. Preserve lowercase ASCII
exactly; the renderer maps either case to glyphs but the parser compares bytes.
Capture target ordinal from `BE16(A6+8)` and descriptor mode from the validated
RAM descriptor. Never assume the displayed password names the just-cleared stage.

`49D60` resolves a mode/ordinal to an actual ROM table string. Pointer-offset
table is `49F8A`; cumulative counts are bytes at `4A4BB`:
`08 35 3F 49 53 5D 7A` (8, 53, 63, 73, 83, 93, 122).
The routine rejects mode 2, subtracts one for modes above 2, maps mode 8 to
lookup group 7, checks ordinal against the group count, then uses the table's
BE16 offset. Require ordinal >=1 independently: original code's upper-bound
check alone is not a safe host API for zero/negative ordinals.

Known campaign counts from the independently validated descriptor inventory:
mode 0: eight; mode 1: 45; modes 3,4,5,6: ten each. The remaining 29-entry
lookup group and other table codes are **not automatically campaign levels**.
Do not expose the entire password table as level selection: it also contains
special commands and unlock codes.

## DB2 password entry and acceptance

The native password screen is called at `167B4` via **`4913E`**. Buffer
`FFE458` holds entered ASCII; `FFE45E` is its length. Parser **`49708`** scans
`0x148/2 = 164` offsets from `49F8A`, compares exactly the typed length at
`49744`, then requires a NUL in the native string at `4974E`. No prefix match
is accepted. A table match reaches `49764`; it writes status 2 to `FFE463`
and the 1-based table ordinal to `FFE464` at `49776`.

**Do not treat `49778` alone as a playable-stage unlock.** Later branches
dispatch special commands or reject unavailable content; some matched strings
are not levels. The accepted exit at `49908` sets screen state `FFE452=2`.
The caller at `167BA` rejects status 1, then **`167C6`** calls `49D08` to
convert `FFE464` into mode D0 and ordinal D1. **`167CC`** is a useful
post-conversion selection hook, subject to validating D0/D1 against a canonical
campaign inventory and confirming the ensuing loader.

Loader **`6F9A`** owns mode/index initialization: stores D0/D1 in
`FF027C/FF027D`, sets pointer `FF0278=FF0292`, decodes the appropriate stream
from table `184400`, copies the selected `5C`-byte record into `FF0292`, and
loads map streams via `164400`. The descriptor after `7010` must agree with
mode, ordinal, map ID and terminal flag. The caller must take this native path;
writing only `FF027C/27D` leaves map and descriptor state inconsistent.

## Safe Extras behavior and remaining validation

Store each actually displayed, valid native continuation code as an unlock;
also record successful native player-entered campaign codes if desired. Keep
the source hook and native target identity with the bytes. For level selection,
present only saved entries whose code still resolves to the same validated
native mode/stage under the effective ROM. A stored password is evidence of
access, not evidence of clearing that stage or earning an achievement.

`games/progress_replay.c/.h` implements a bounded continuation request. It queues
until the **native password screen** is reached, without changing PC or bypassing
the game's parser, fades, descriptor decode, map loader or stage initialization.
The host can show the status text while the user navigates there. No automatic
guest-title navigation is claimed.

DB1's qualified prefill hooks are `1DB7E` (immediately before native typed-code
renderer `1DEEE`) and `1DB88` (password input loop). The only changes are four
code bytes at `FF0036`, typed-length low word D4=4, and cursor low words D2/D3=0.
Original setup itself uses D2/D3=0 at `1DB84/1DB86`. Selecting the first ordinary
letter while D4=4 branches through `1DBE2/1DBE6` to the original parser before
a fifth letter can be written. The module finds the physical A/B/C whose live
button remap `RAM[28+i]` equals logical action1; if none exists it refuses to
guess. Its frame callback requests release three frames, press three frames,
release three frames, then restores normal input. The adapter translates abstract
physical A/B/C masks1/2/4 to its controller masks. `1E7F6` explicitly waits for
release and then a new press. `1DD90` confirms the native parser's stageword and
descriptor match; `1950` confirms entry into the native loader. A later typed-code
edit causes failure rather than another silent overwrite.

DB2 prefill is intentionally later: at native parser entry `49708`, supply only
`FFE458[0..5]` and length `FFE45E`, once, then let the original comparison run.
The request waits for the user to submit normally in the password screen; no
unqualified automatic submit mapping/cursor manipulation is introduced. Accepted
branch `49908` must have status2 and the expected one-based table index. The
native loader at `6F9A` must receive matching D0/D1. It then performs its ordinary
descriptor and map initialization. All replay requests time out after1800 guest
frames and can be canceled. Terminal states stop injecting input. Root should
call the frame callback once per guest frame and use its input override only
while explicitly requested; it must not latch a pressed pad after override ends.

Tests exercise queuing without unrelated RAM writes, native buffer prefill,
button-remap selection, exact pulse duration/release, no repeat overwrite,
parser/loader acceptance, cancel and timeout. These are synthetic contract tests;
an integrated native runtime replay capture is still required before claiming
end-to-end level selection has been visually verified. No general direct-PC jump
or arbitrary RAM stage mutation is established or used.

Current evidence proves the shared result-code renderers and native parsers by
disassembly; it does **not** establish an exhaustive inventory of every ending,
bonus, tutorial, hidden-message or alternate code-display screen. DB1 MHRI is
a known additional screen covered by its separate `28F1A` hook. DB2's single absolute call to
`49D60` is the result renderer; embedded literal codes or another decoder would
not be found by that reference search. An exhaustive promise must remain open
until those native routes are captured, with screen-specific hooks added.

## Storage module

`games/progress_session.c/.h` exposes an immutable observer receiving the RAM
view and A0–A7 values, a current displayed-code candidate, an explicit save
action, filtered saved-entry enumeration, and native-code display text. Its
observer captures DB1 `1BECC`, hard-ending `28F1A`, successful manual entry `1DD90`, plus DB2
`58EF2`. The adapter must dispatch these exact PCs and the documented reset
points. Enumeration revalidates every saved code against the current immutable
ROM table, so corrupted or stale logical entries are not exposed as levels.
DB1 special mode is represented in host records as mode 4, ordinal 1–17
(original indices 80–96). DB2 enumeration currently permits only the established
campaign groups 0,1,3,4,5,6. Display observation creates a candidate; it does not
silently save or grant achievements. `dyna_progress_session_save` atomically
persists a copied store and updates in-memory progression only after success.

`tests/progress_session_test.c` verifies native pointer and descriptor checks,
DB1 encoded-to-display text, DB2 next-stage target selection, filtered enumeration
after ROM changes, and rejection of mismatched source pointers. It passes the
same strict GCC warning configuration as the store test.

`games/progress_store.c/.h` is a pure host store: no guest dependencies, no
achievement effects and no generated passwords. Version 1 file has fixed-endian
fields, game ID, 64-hex effective ROM SHA256, bounded count (256), explicit raw
code lengths, mode/stage, source PC, and an FNV-1a corruption checksum over
header plus payload. Identity mismatches, truncation, trailing garbage, invalid
records, duplicate serialized records, unsupported version and checksum failures
leave the caller's state unchanged. The checksum detects corruption; it is not
an authenticity signature, so native code validation remains mandatory on load.

Save validates the entire store before opening a same-directory exclusive
temporary file. It checks write/flush/close, commits the file with Windows
`_commit`, then replaces through `MoveFileExA(REPLACE_EXISTING|WRITE_THROUGH)`.
No path pre-delete occurs; failed replacement keeps the previous valid store.
The POSIX branch uses fsync then same-directory rename. Parent directory fsync
is not implemented in that fallback; Windows is the qualified target. Concurrent
independent writers are not merged, so the app should own one store and serialize
mutations. Windows paths use the app's existing narrow-path convention; Unicode
path conversion is a separate integration consideration.

`tests/progress_store_test.c` passed GCC C11 with `-Wall -Wextra -Werror`,
including actual Windows locked-file replacement failure, valid-file preservation,
successful replacement, idempotency, invalid-record rejection, game/ROM mismatch
and checksum corruption with unchanged in-memory state.
