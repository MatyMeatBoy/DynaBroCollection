# Recomp Achievements in DynaBroCollection

The 0.5.0 Beta 1 game executables support RecompLauncher's event API. Each executable sends live events through the SDK's local session channel when RecompLauncher supplies `RL_SESSION`. RecompLauncher owns account progress, eligibility, sync, and presentation. Offline play continues to record local achievements in the existing profile. Loading that profile sends no launcher unlocks. A goal earned offline can count in RecompLauncher when the player meets its requirements again during a connected session.

| Game | Candidate tile ID | Launcher goals | Beaten requirement |
| --- | --- | ---: | --- |
| Dyna Brothers | `dynabrocollection--db1` | 76 | Every Normal stage, which Hard victories also qualify for |
| Dyna Brothers 2 Special | `dynabrocollection--db2` | 99 | Every Story stage |

DB2's six campaign completion achievements require all stages of the corresponding campaign in one connected game process for the launcher award. Its local profile still accumulates campaign completion across play sessions. The events API gives the game no account unlock history. Keeping launcher completion based on live play prevents an offline profile from satisfying its prerequisites.

In the DB2 launcher list, Story stages 1–44 are progression goals and Stage 45 is the win goal. Beaten requires all 45 Story goals, which can be earned over multiple sessions. Mastered requires all 99 launcher goals across Practice, Story and Original I–IV, including the six campaign completion goals. [Recomp Achievements list format](https://github.com/elripalda/RecompAchievements/blob/faf094acdf1efb57d8db7c8187b4b47aa1501805/docs/SPEC.md)

The game reports a save load after a native password replay reaches the stage, or when a valid SRAM file loads. Diagnostic, scripted and diagnostic-replay sessions remain excluded from game-generated progress. The DB2 observer rejects battle events while the native cheat flag is active and reports that session as Casual when it observes the flag. The launcher decides the final classification.

## List registration and beta testing

The two format-1 event lists are in [assets/recomp-achievements](../assets/recomp-achievements/). The CMake `DynaAchievementAssets` target places them in `<build>/assets/recomp-achievements/`. Production lists are keyed to the deployed RecompLauncher listing ID followed by `--db1` or `--db2`. The candidate base ID is `dynabrocollection`. The version filter matches the installed tag `v0.5.0-beta.1` or `v0.5.0`.

For a developer override, copy each JSON to the launcher's data directory at `achievements/dev-lists/<actual full tile id>.json` and set the copied list's `game` field to that tile ID. Developer builds and manually added copies can get a `local-` or `own-` prefix, and the launcher can add a suffix to distinguish duplicate installations. For example, an install assigned `local-dynabrocollection` reads `local-dynabrocollection--db2.json` for DB2. Omit `versions` from the developer copy when the install uses the `local` or `own` tag. Start each individual tile with Developer tools enabled to see the SDK `hello` and live `event` messages in the dev log. [RecompLauncher developer imports](https://github.com/elripalda/RecompLauncher/blob/57151432aeb48698a89148b77baed47f78419b03/src-tauri/src/devtools.rs) · [RecompLauncher achievement lists](https://github.com/elripalda/RecompLauncher/blob/57151432aeb48698a89148b77baed47f78419b03/src-tauri/src/ach.rs#L310)

Reading a developer override does not require uploading the lists. The real launcher supplies `RL_SESSION` when a signed-in account has Recomp Achievements tester access, and its normal account synchronization still applies. The local pipe test below uses no account or network service. Publishing the production lists, activating the beta for testers and verifying cloud awards and ranking remain launcher-side steps. [RecompLauncher session setup](https://github.com/elripalda/RecompLauncher/blob/57151432aeb48698a89148b77baed47f78419b03/src-tauri/src/ach.rs#L547) · [RecompLauncher beta guide](https://recomplauncher.com/sdk)

To verify list generation, run `python tools/generate_recomp_achievement_lists.py --check`. Build `DynaRecompAchievementSessionTest` and run `ctest -R recomp_achievement_channel_contract --output-on-failure` from your build directory to test both games against a local pipe or socket with simulated native battle events. The test covers reporting a previously unlocked local goal only upon earning it again, isolated completion requirements, save and Casual notices, and offline profile progression.

## Content and licenses

The DB1 launcher list uses the established game event IDs and newly written titles and descriptions. It carries no RetroAchievements badge images, copied player history, or rule definitions. The existing offline DB1 metadata and badge pack came from the public catalog credited to Tybis. Its [provenance file](../assets/achievements/db1-provenance.json) identifies the originals, and public access to the [RetroAchievements pages and media](https://retroachievements.org/terms) does not establish a redistribution license. Confirm rights for that existing pack before redistributing it.

The DB2 launcher list is based on the collection's own goals. The [DB2 offline badges](../assets/achievements/db2/provenance.json) have their own project license. The launcher list format version 1 shows a shared star icon and has no custom badge field. [Recomp Achievements specification](https://github.com/elripalda/RecompAchievements/blob/faf094acdf1efb57d8db7c8187b4b47aa1501805/docs/SPEC.md)

See [SDK pin and notices](../third_party/recomp-achievements/UPSTREAM.md) for the exact vendored revision and licenses.
