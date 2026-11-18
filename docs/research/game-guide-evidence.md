# Game guide evidence

Checked 2026-11-18. The guide is English and immutable. Game IDs are 1 (Dyna Brothers) and 2 (Dyna Brothers 2 Special). Basic and advanced strategy have exactly two pages per game. Examples are ordinary tactical applications, not claims of a recorded winning run.

## Primary sources

- A: [Sega DB1 introduction and character profiles](https://vc.sega.jp/vc_dyna/), character section and system overview.
- B: [Sega DB1 instructions](https://vc.sega.jp/vc_dyna/instruction.html), numbered steps 1 through 6.
- C: [Sega DB2 Special character profiles](https://vc.sega.jp/vc_dyna2/), dinosaur-team section.
- D: Original DB2 binary prompt strings, recorded in `translations/db2special-en.json`. The file explicitly says binary-only / scene-pending. These establish command vocabulary, not fully tested gameplay effects.
- E: Local traced DB1 routines and qualification contracts in `docs/db1-achievement-contracts.md`, especially Rain, disasters and timeout extinction. This is code evidence, not a measurement of combat statistics.
- F: [Sega DB2 official manual link](https://www.sega.jp/mdmini/manual/pdf/m_jp_dyna-brothers2.pdf), linked from [official title page](https://www.sega.jp/mdmini/soft/dyna-brothers2.html). Retrieval failed (browser timeout and HTTP 403); no claims rely on unread contents.

## db1-units

A verifies all five displayed species and their qualitative roles: Stego reproduction, Tricera defense, Allo interception, Tyranno attack, P-chan egg eating. B supports the associated ordinary usage advice. Display spelling follows the existing English credits translation; role descriptions are short original English summaries.

## db2-units

C verifies the six displayed species: Stego breeding, Tricera durability, Allo interception, Tyranno attack, Ptera flight across terrain and P-chan egg eating. It also identifies Tyranno as strongest in attack. No DB1 Ptera entry is shown.

## db1-unit-stat-contracts

The post0.1.5 DB1 guide has five species pages. Initial vitality, strict reproduction thresholds, parental vitality costs and the five-by-five opponent-specific combat drain matrix are verified through original routines and species/credits binding in [DB1 unit contracts](db1-unit-stat-contracts.md). The typed reader and every numeric cell pass private original and qualified English image checks. Initial vitality is not maximum HP; parent cost is not player power; per-routine combat drain is not DPS. The two Advanced pages now explain the shared vitality budget and native terrain-route preparation.

The actual Extras pixel compositor was rendered at427x240 for all five unit pages, including scrolling; component bounds tests also cover320x224 and smaller fallback canvases. Captures remain private under local/guide-unit-review. Existing native egg-chooser images illustrate unit selection, not an invented recorded matchup.

## Statistics still outstanding

Universal movement/attack timing, incubation, player creation/upgrade power prices, defense ratings, maximum/current vitality and complete difficulty modifiers remain unqualified. DB2 numeric statistics remain unchanged and unverified while DB2 development is paused.

## db1-hud

B supports power spending, eggs and grass; A and E support the environment-device/time threat. E records the timed extinction dispatch. The guide avoids unverified HUD coordinates, icon identifications, countdown units and exact command costs.

## db1-hazards

B establishes starvation, hungry idle Allo, egg protection and Tyranno terrain restrictions. A/E establish disasters; E distinguishes visual quake, tsunami and destructive meteor routines. Damage radii and immunity claims are omitted.

## db1-basics / db1-advanced

B supports the economy, route preparation, local defense and attack sequence. Advice to inspect base/front and reserve power is editorial tactical guidance, not a hidden rule or a guaranteed solution. No exploit, password reward farming or AI-lock tactic is taught.

## db2-hud

D provides egg creation/upgrading, Advance/Gather/Brawl/Breed, Guide, Mark, Plant/Rain/Dry and Bolt/Meteor/Quake labels and target prompts. The guide does not infer numeric effects. C supports terrain-aware unit selection and egg vulnerability. DB2 terrain-state hazards, food depletion timing, exact HUD symbols and disaster effects still need primary or native execution verification; DB1 effects are not silently applied to DB2.

## db2-basics / db2-advanced

C supports role combinations. D supports selecting targets for Guide and Gather. C's story overview identifies progressive teaching stages. All example situations are strategy suggestions and require appropriate units/orders to be unlocked in the chosen stage.

## Native screenshots

`DynaGuidePage.capture_key` and `capture_scenario` are requests to the native engine capture pipeline. They are not evidence that an image exists. This content module does not create or fake screenshots, and no external promotional image is represented as this build's output. Capture the real renderer, retain the native HUD and record game, stage, input script, frame, ROM hash and image hash in a capture manifest. Only publish an image after checking it actually demonstrates its page's example; a generic gameplay frame must be captioned as an overview. No numeric assertions can be inferred from a decorative screenshot.

## DB1 engine captures (2026-11-18)

Seven actual native 320x224 PNGs now exist under `assets/guide/db1-*.png`. Their exact engine frame requests, image hashes, binary/ROM hashes and observational scope are in `assets/guide/db1-capture-provenance.json`; reproducible guest inputs are in `assets/guide/db1-captures.input`. Execution used an isolated copy of `build/dev/DynaBrothers1AOT.exe` at `local/guide-db1-bin/DynaBrothers1AOT.exe`, with development DLLs supplied by PATH, an isolated profile, English partial translation, native 4:3 and a 10,000-frame finite probe. `local/guide-db1-captures/probe.json` reports passed, return code zero and no missing dispatch/capture failures. Each included frame was visually inspected. PNGs are copied byte-for-byte from engine output; no generated artwork, compositing or screenshot enhancement was applied.

The HUD frame (6000) precedes any diagnostic write. Subsequent frames use only the declared extra-power fixture (`WRITE_RAM32 0xFF7BD6 0x00999999`) followed by real guest menu controller inputs. Species selection, eggs, incubation, movement and Rain are executed by the engine; no actor records, ROM instructions, program counter or terrain records were patched. These are illustrative setups, not earned campaign resources or proof of an optimal strategy. DB1 page examples explicitly label the extra-power setup.

Capture coverage is deliberately narrower than the full written guide: units shows the native egg chooser; hazards shows the grass/water boundary and Rain command; the basic and advanced examples show economy/defense/route inspection at the home ground. No included image claims to show an enemy raid, successful assault, starvation, extinction, tsunami or every dinosaur species. Basic/advanced page text was adjusted to match these genuinely visible overview examples. Combat outcomes and numeric species statistics remain outstanding. Screenshots do not validate those numbers.
