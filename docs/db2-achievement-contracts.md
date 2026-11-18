# DB2 Special local goals

This is a custom local set, not an official RetroAchievements set. The public game entry is [9844](https://retroachievements.org/game/9844). The code observes the qualified Japanese Special Virtual Console layout and its guarded English prompt patch without modifying either input image.

| Campaign | Loader mode | Stages | Local stage IDs |
| --- | --- | --- | --- |
| Practice | 0 | 8 | 2000001–2000008 |
| Story | 1 | 45 | 2000101–2000145 |
| Original I | 3 | 10 | 2000301–2000310 |
| Original II | 4 | 10 | 2000401–2000410 |
| Original III | 5 | 10 | 2000501–2000510 |
| Original IV | 6 | 10 | 2000601–2000610 |

Six completion goals (`2000900 + mode`) require every stage goal for that campaign in the validated local profile. Clearing only the final stage or entering a password does not satisfy a completion goal. There are 99 goals worth 1025 local points. Versus difficulty names are not additional hard campaigns; versus, demonstrations and special independent maps are excluded.

The loader at PC `006F9A` stores mode/stage in RAM `FF027C/FF027D`, uses the mode pointer table at `184400`, and copies a `0x5C`-byte descriptor to `FF0292`. The observer validates its identity, map ID and terminal-stage flag against the decoded campaign tables.

Qualification requires the actual battle call at `003BB0`, its return at `003BB4` with result byte `FF9C86 == 1`, and the genuine win branch `003CB2`. New loads, title, defeat and inconsistent descriptors cancel an armed battle. The alternate cheat route and magic word `FF04C2 == 5548596F` cannot award goals. Result screens alone are not an unlock hook.

`tests/db2_achievement_events_test.c` covers positive stage wins, invalid campaign/stage/descriptor identity, defeat, cheat and diagnostic exclusion. `tests/achievements_test.c` covers cumulative milestones, cross-game separation, transactional persistence, corrupt profiles and notification order. These are event contracts; they do not certify playing all 93 stages end to end.

The six original badge portraits and numbered variants are documented in `assets/achievements/db2/provenance.json`, with their editable vector sources, hashes and license scope.
