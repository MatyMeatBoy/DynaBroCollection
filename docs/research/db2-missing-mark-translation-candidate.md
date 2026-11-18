# DB2 missing-mark prompt: bounded investigation

Investigation on 2026-11-18. This is a candidate for a subsequent English edit,
not a translated or visually qualified scene. The 22-entry catalog and output
CRC `DAC9BAFC` are unchanged.

## Original text and call site

The qualified original Special dump contains the following 14 bytes at `$9A72`:

```
40 CF B0 B8 40 B6 DE 01 C2 B2 C3 CF BE DD
```

They decode as `@マーク@ガ` / `ツイテマセン` in the game's half-width
Japanese convention: the selected creature has no mark. A concise candidate
wording is `NO_MARK` / `SET` (`NO_MARK\x01SET`, padded within the existing slot).
The byte at `$9A80` is NUL; `$9A81` is alignment padding. Neither byte needs an
edit. This candidate is absent from `translations/db2special-en.json`.

The original control flow is explicit:

| Address | Instruction / meaning |
| --- | --- |
| `$9A48` | Load selected entity pointer from `$24E8(A5)` |
| `$9A4C` | Load entity word at `+$0A` |
| `$9A50` | Branch to `$9A82` when nonzero (marked entity) |
| `$9A54` | Push sound `$A3`, invoke original F-line `$FF2F` |
| `$9A5C` | Set UI refresh byte `$FF430E` |
| `$9A62` | Push pointer `$9A72` |
| `$9A66` | Push UI argument `$0028` |
| `$9A6A` | Invoke original F-line `$FF2A` |
| `$9A6C` | Remove six argument bytes, return through `$9ACC` |

F-line `$FF2A` resolves through the original table at `$6BC` to `$163C`.
The existing translated `pick_egg` prompt also invokes `$FF2A` with argument
`$0028` (call sequence immediately before its `$81B6` text). This establishes
a shared call interface, but does not by itself establish a rendered capture
of the missing-mark text.

## Native reachability experiment and negative result

The existing `unpause-gameplay.input` route was extended only in private
diagnostic scripts under `local/language-pass/`. With `A5=$FF056E`, these
prepared the selected entity's mark word at `$FF9C98`, installed `$9A48` in
the action callback at `$FF2A6E`, and tried A/B input and action-state byte
`$FF2A88`. The entity pointer observed in the native RAM dump was `$FF9C8E`.

Four native attempts completed their finite frame budgets with no reported
interpreter fallback; the missing-mark text did **not** appear in the captures.
The route instead showed the normal gameplay image or `PRESS START`. Therefore
the callback preparation is not a valid positive rendering scenario. In
particular, successful probe exit status must not be interpreted as successful
translation or branch coverage.

Private evidence directories:

- `local/evidence/language-missing-mark-jp` (18,000 frames)
- `local/evidence/language-missing-mark-menu` (18,100 frames)
- `local/evidence/language-missing-mark-b` (18,100 frames)
- `local/evidence/language-missing-mark-mode-a` (18,100 frames)

These runs used native executable SHA256
`a80fba2ee89673a5a333bd26ba8de72f169048d044ba94c5bb59942f96acb268`
and input-ROM SHA256
`157c75e691528da42abf87e1074d26dca96c14151026be1bd4f8d288889be2db`.
Screenshots and diagnostic RAM are private, not committed.

## Next bounded gate

Reach the original unmarked-creature action in a normal gameplay context and
observe `$9A62` / the Japanese prompt before applying the candidate. Then add
the exact 14-byte guarded entry, compare English and Japanese captures in native
and independent reference, and exercise wrong-byte, capacity and overlap
negative fixtures. Do not promote the callback attempts above to a regression
scenario until the original branch is demonstrated.

An additional inventory check found that the current 22 catalog slots do not
share a universal immediately-following-NUL contract: `upgrade_egg` is followed
by `$E0`, `gather_spot` by `$2A`, and the other 20 by NUL. Consequently no
universal NUL guard was added based on this candidate's layout.
