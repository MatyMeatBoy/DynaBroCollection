# DB1 native progress qualification

Date: 2026-11-18. DB2 remains outside this qualification.

## Native replay verified

An actual engine run loaded Normal stage2 using the original Password screen,
original password parser, and original stage loader. No replay implementation
fix was required. The diagnostic queued native table index21 (OSCK, encoded
bytes 3A 4A 0A 2A), read directly from the effective ROM. It did not set the guest
PC, stageword, descriptor pointer or map state. Only the existing replay module
prefilled the four password bytes, typed-length/cursor registers, and supplied
its bounded physical controller pulse.

The observed native sequence was:

| Native hook | Status | Elapsed guest frames |
| --- | --- | --- |
| 1D7B6 Password scene entry | WAIT_PASSWORD | 0 |
| 1DB7E typed-code renderer entry | PREFILLED | 15 |
| 1DCEC original parser | SUBMITTED | 19 |
| 1DD90 after original parser success | ACCEPTED | 19 |
| 1950 native loader entry | LOADED | 67 |

The actual remap selected physical C (abstract mask4). Frames16-18 released,
19-21 pressed, and22-24 released. The original parser wrote stageword0015 and
canonical descriptor pointer00FFB3F0. Read-only scripted assertions confirmed
both values after900 more frames. The resulting stage/map screenshot was
visually inspected; the run completed6000 guest frames without runtime errors.
LOADED still means native loading started, rather than an independently audited
full map initialization contract; the later screenshot establishes playable
stage presentation for this specific Normal2 route.

Private evidence:

- local/progress-agent/replay-index21/probe.json
- local/progress-agent/replay-index21/run.log
- local/progress-agent/replay-index21/progress-replay-native.png
- Copied binary: local/progress-agent/db1-replay-test.exe
- Binary SHA256: a267aa5bdb3b38378e9ee28ac17450153e5711550f39d0b487b52a694388950f
- Prepared ROM SHA256: ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261

Reproduction uses DYNA_DB1_REPLAY_TEST_INDEX=21 and:

```powershell
python tools/native_probe.py local/progress-agent/db1-replay-test.exe local/roms/db1-en-950A8FC3.md --script games/db1/scenes/progress-replay-native.input --output local/progress-agent/replay-index21-repeat --frames 6000 --timeout 120
```

## Native queued cancellation verified

The same copied executable, ROM and native route with
DYNA_DB1_REPLAY_TEST_CANCEL_AFTER=2 reached CANCELED at elapsed2, pulse0,
override0/buttons0. The original Password screen remained empty and interactive
in the inspected screenshot. No prefill or parser submission occurred. The run
completed5600 frames. Evidence is local/progress-agent/replay-cancel/.

## Native navigation and diagnostic boundaries

The native title starts on NEW GAME. Three separate DOWN presses, each followed
by release time, select PASSWORD; A enters it. One DOWN selects CONTINUE, which
starts the current stage. The scene progress-native.input establishes this route
without guest memory writes, state loading or PC changes.

The runtime diagnostic is dormant unless DYNA_DB1_REPLAY_TEST_INDEX contains a
strict decimal qualified DB1 table index. It resets on runtime start, bounds
checks actual effective-ROM reads, excludes reserved indices8-19, and uses the
existing replay API. QUEUE_TITLE=1 optionally queues at existing native title
hook1C5A4 for timeout tests; otherwise it queues at Password entry1D7B6.
CANCEL_AFTER invokes existing host cancellation after the selected elapsed
frames. Diagnostic logs include status, native PC, elapsed/pulse frames, physical
input override, and actual stage/pointer/buffer at transitions.

Queueing at startup would expire1800 frames before this script reaches the
password scene (over4500 boot/navigation frames). This was a diagnostic setup
limitation, not evidence that a real Extras request queued at an arbitrary
later time fails. Product UX should state that requests expire after30 seconds
of guest time, or use an intentional navigation-aware bound if desired.

This test bypasses the host Extras selection to isolate native replay. It does
not establish that a code was displayed, saved via Save Game, persisted/reloaded,
or selected through actual Extras. Those host UX steps remain separate work.
Existing replay synthetic tests also passed, but are supplementary to this actual
engine evidence. No proprietary ROM or screenshot was added as a public asset.

## Native queued timeout verified

With DYNA_DB1_REPLAY_TEST_INDEX=21 and
DYNA_DB1_REPLAY_TEST_QUEUE_TITLE=1, the request queued at native title hook
1C5A4 and expired at elapsed 1801, status FAILED, pulse 0,
override 0/buttons 0. No prefill, press or parser submission occurred. The
inspected screenshot remained at the title with NEW GAME selected. The run
completed 7000 guest frames without runtime errors.

Evidence: local/progress-agent/replay-timeout/{probe.json,run.log,
progress-timeout-title.png}. Binary: local/progress-agent/db1-replay-timeout.exe,
SHA256 c9ca2bcfe54a6cb717a15e26ddc2be0eda8c722106c76e92a429e6d3c13eca6f.
Use games/db1/scenes/progress-timeout-native.input with 7000 frames.

## Proposed host Save Game and Extras qualification

No additional hooks for this proposal have been implemented. An actual clean
campaign win/loss result should reach observed display hook 1BECC. A bounded,
opt-in host driver can wait for session_current to become valid there, then
invoke the public dyna_progress_runtime_action("dyna.save") once and record its
return, status, saved count and created private-profile file. It should capture
the displayed code and the paused Extras notice in separate images. Driving
normal host input after the screenshot is preferable to auto-saving inside the
CPU instruction hook: opening a modal inside instruction execution can defer
presentation until a later frame and obscure exactly when the save occurred.

After restarting with that same isolated profile, a host test should verify
count/item and navigate the existing Extras semantic inputs: open; confirm to
Levels; confirm the saved entry; close Extras. That exercises the actual
provider launch callback, which currently deliberately returns 0 to retain
navigation guidance. Navigate natively to Password afterward and observe the
already qualified parser/loader sequence. The initial request must originate
from this real provider callback, without the diagnostic table-index request.
A second fresh profile must show no saved entries. Matching native code in the
record and effective-ROM identity must remain part of the assertion.

A manually accepted password is also a valid observer candidate at 1DD90, but
it is cleared at loader entry 1950 about 48 frames later. Auto-saving that
candidate in a diagnostic would establish persistence of accepted native codes,
not the user's requested Save Game action while a result code is shown. Keep
those two claims distinct. No guest win/loss flag, result stage or PC mutation
should be used as campaign-playthrough evidence.
# DB1 Save Game callback qualification, 2026-11-18

Private future-iteration work; excluded from immutable v0.1.5/r2.

Actual engine Save passed using local/progress-agent/db1-save-test.exe,
SHA256 42651db20672eca0d251cf131d832a412908b4383bbd49b1ae2cb36f15c9948e.
Effective ROM SHA256 ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261.
English profile used consistently for save and separate-process reload.

The successful fixture is save-type1.input: the established prepared-win
condition from practice-completion-diagnostic.input, with initial stageword20
and descriptor type1. This is explicitly a prepared result, not campaign play.
No PC changes or guest instruction/ROM patches occur. Ordinary native result
renderer reached 1BECC, displayed NEXT PASSWORD OSCK and yielded a validated
Normal2 candidate. On the next host input-filter frame the public Save Game
action returned PROGRESS SAVED. A fresh disk store/session validated count1 and
identical code/mode/stage. The actual file is105 bytes, no temporary file remains.
Evidence: save-type1/{run.log,probe.json,progress-save-result.png,profile/}.
10500 frames completed without runtime errors.

A second process reused the exact same isolated profile. Runtime startup loaded
the stored record. Diagnostic mode reload reported PASS count1 source01BECC
mode1/stage2. Public Extras API navigated home->Levels, compositor displayed
MODE1/STAGE2 and PASSWORD OSCK, and confirmation invoked the real registered
provider launch callback. It queued the same validated code; diagnostic then
canceled and closed, avoiding any forced guest navigation. Evidence:
save-reload/{run.log,levels.png}. A separate fresh profile reported PASS count0
(save-empty/{run.log,probe.json}). This proves public callback/compositor wiring,
not physical SDL events or natural native-menu insertion.

Important unsuccessful fixture evidence retained: practice type4 reaches its
separate completion congratulations with no code; type0 reaches result stats
without a password; Normal2 native-load plus the initial-stage outcome writes
did not terminate that different map. None produced a false Save PASS.
Original dispatch proves type0 at1C6E passes arg1 to result1B9DA, skipping code;
type1 at1C7E passes arg0, reaching1BBE8->1BEA8. Practice type4 is separate1C9C.

Dormant diagnostic source is preserved as save-test-runtime.patch and full
save-test-runtime.c (SHA256 cc24a5ee3b95afccdcb5d54a5cf41bb15e1870f928131e0f26ba526275bdfd79).
Tracked runtime was restored to HEAD for packaging. No production UX changed.
DYNA_DB1_SAVE_TEST=save|reload|empty triggers only this DB1 host diagnostic.
The guest retirement hook only marks an actual valid1BECC candidate pending;
Save/file I/O/Extras rendering/navigation occur in player0 input_filter.
main.c calls collect_sim_input immediately before genesis_sim_step (4095/4265),
so the action is outside the 68000 instruction block. Code resets teststate on
runtime start, runs once, bounds-checks captures and uses private profile paths.

# Proposed original-style result choice (read-only design)

1BECC is too early to show an overlay: it writes four password tiles before
native result sprite construction and fades finish. Stable result input loop
is1BD20..1BD7A; at1BD6C original code calls2B38, then tests D0==4 at1BD72 and
branches1BD7C on that native START action.1BD7C clearsFF0018 and begins native
audio/fade/return, ending1BE18. A future host latch should require a valid
source_pc1BECC candidate AND arrival at stable1BD6C (or1BD20), then clear on
1BD7C, stage init1950, title/frontend reset and runtime reset. Accepted-password
candidate1DD90 must never activate it; practice completion has no1BECC candidate.
Hard-ending MHRI28F1A is a different screen and needs its own lifecycle design.

Screenshot native viewport is320x224 centered at x40 in400px output. Observed
result box spans approximately native x68..248,y20..126; password row at y112;
dinosaur sprite row y142..174; original PRESS START BUTTON box approximately
native x84..238,y185..200. Suggested two-line choice panel occupies x80..240,
y178..215, replacing only the original prompt in the host framebuffer and
preserving code, statistics and sprites. Use native-style8px monospaced white/
cyan glyphs, thin double border and triangular cursor. Coordinates are from
this inspected screenshot, not a universal layout contract; test original/
English,4:3/16:9 and multiple UI scales before adopting them.

Host input filter can consume Up/Down and confirmation while SAVE GAME selected,
invoke existing public Save at next host frame, render Saved/failed feedback,
and leave original stage/PC untouched. CONTINUE should pass a bounded physical
START pulse through the existing controller path so the original1BD72 branch
owns exit; never change PC or stage directly. Require release before recognizing
navigation/confirm to avoid a held battle button immediately choosing an option.
Default selection CONTINUE preserves quick normal flow. The original wait loop
and animations may keep running, because guest receives no START until Continue.
The host overlay needs the same native-viewport transform as the game compositor;
render after native picture assembly, before final scaling, never write VDP/ROM.
No production overlay or input changes implemented yet.

## Result prompt state implementation (2026-11-18, after v0.1.5)

The new pure progress_ui state is activated only by an independently validated
1BECC candidate followed by stable native input hook 1BD6C. Repeated loop hooks
preserve selection and feedback. Accepted-password hook 1DD90 cannot activate it.
Native exit 1BD7C, stage initialization, password entry and title/frontend resets
clear it. Visibility is independent of guest pause state.

The default is Continue. All buttons must first be released. Up/Down changes the
two-choice selection; physical A/C or START confirms on an edge. Save calls the
shared atomic save helper from the host frame input filter, keeps the result
screen active and exposes PROGRESS SAVED or SAVE FAILED feedback. A held confirm
does not repeat the write; releasing and pressing again permits retry. The
existing host-menu Save action retains its Extras notice/open behavior.

Continue supplies three frames of physical START then release through the normal
pad path. It changes no PC, stage, descriptor, ROM or VDP data. While awaiting the
native exit, the state suppresses ordinary guest buttons to avoid a stuck or
second confirmation; native exit clears that filter state. Prompt getter outputs
are optional and read-only. The original native animation/input loop keeps
running throughout visibility.

TDD evidence: an initial no-op implementation failed the activation assertion in
progress_ui_test.c. The implementation then passed C11 -Wall -Wextra -Werror,
covering early drawing, repeated stable hook, accepted-password exclusion,
release guard, selection, held-confirm write suppression, failed-save retry,
feedback, bounded Continue pulse, and initialization/title/exit reset. Runtime
integration compiled to a private object with the same strict warning policy;
a pre-existing misleading-indentation statement was split onto separate lines.
No root executable was relinked by this implementation step. Native integrated
screen captures and failed-write/Continue behavior remain the next qualification.

## Production prompt input qualification before composed capture

Copied integrated executable SHA256
aa993340241f33facd4e0c1608d500ebb9ba634260728f57705a7936afb6b3d2
completed the prepared-result production input route in English and Japanese at
original 4:3, 12500 frames each. No Save diagnostic environment was set. Actual
Down/A selected and saved once, Up/A continued, and the original loader reached
stageword21/pointer00FFB3F0. Each isolated profile contains a105-byte native-code
store, with its corresponding effective ROM identity (Englishad9a9045...,
Japaneseb4918a91...). Evidence: prompt-en-43/ and prompt-ja-43/.

An actual Windows failure probe first saved normally, then held the previous
store open without delete sharing before a second real A confirmation. Runtime
reported SAVE FAILED; the105 prior bytes were unchanged and only one successful
Save was logged. Evidence: prompt-write-failure/{run.log,probe.json,lock-test.json}.
This covers production atomic-replacement failure handling, not only a mock.

These runs qualify input/persistence/native Continue, but their scripted PNGs
cannot yet qualify overlay presentation. Existing runner_write_screenshot_file
writes raw s_framebuf, while host overlays are drawn later into s_present_buf.
Thus the captured PNGs intentionally show the old native PRESS START prompt,
even though production prompt input and saving work. Composed-output capture is
required before making any visual completion claim. No runtime/state fix was
inferred from this instrumentation boundary.

The same copied production executable also passed the actual-input route in
English and Japanese with a real2560x1440 SDL window and16:9 presentation,
12500 frames each, including Save/store/native Continue assertions. Evidence:
prompt-en-1440-raw/ and prompt-ja-1440-raw/. Mixer PCM signal was present in
both runs; uncapped-runtime audio queue overflow is expected under this speed
probe and is not a physical playback claim. The same frame hashes as the
corresponding4:3 runs confirm that presentation sizing did not alter guest state.
Their scripted PNGs still exclude host overlays for the same capture-boundary
reason, so composed visual qualification remains open.

## Composed production presentation qualification

Opt-in host-overlays capture resolves the instrumentation boundary without
changing default raw captures or the guest framebuffer. Copied executable
SHA256362a539296176823125a80b1daa5a0b877f11d160430dad1ae8f6c76c21b8273
passed exact previous input routes in English/Japanese real2560x1440 windows
and one English4:3 run,12500 frames each. Native game and audio hashes match
the previous raw captures exactly in every pair. Automated image comparison
found every changed pixel inside the160x45 prompt rectangle only: native
(80,176)..(240,221), or canvas(133,184)..(293,229) at427x240. The displayed
password, upper statistics, and animated dinosaur row remain byte-identical
outside that rectangle. Idle/defaultContinue, Save selection, saved feedback
and Continue selection images were visually reviewed in all three runs.
Evidence: prompt-en-1440-composed/, prompt-ja-1440-composed/,
prompt-en-43-composed/, and prompt-composed-comparison.json.

The actual locked-file failure case was repeated with composed capture. The
inspected prompt-save-failed.png clearly shows SAVE FAILED while retaining the
code/stats and Save selection. Previous105-byte store remains identical;
one success and one failure were logged. Evidence:
prompt-write-failure-composed/{run.log,probe.json,lock-test.json,
prompt-save-failed.png}.

Observed Continue presentation limitation: three frames after confirmation,
the native exit has already cleared the prompt and the original PRESS START
BUTTON region is visible during the native transition. The composed image is
identical to its raw counterpart at that point. This is not a blocked Continue:
subsequent native loading and stage/pointer assertions pass. Any cleanup must
handle the native exit/fade lifecycle rather than merely keeping the getter
visible during its three-frame input pulse. No speculative presentation fix
has been applied; the root integration owner was notified with this evidence.

## Bounded Continue presentation correction

The observed native-prompt reappearance motivated an exiting presentation state.
At native exit1BD7C, active input override stops immediately, but the result
presentation remains visible with CONTINUING feedback through the original fade.
The three-frame START pulse is unchanged before exit; all guest input passes
through normally after exit. The getter remains read-only and is visible during
both continuing and exiting. A new observed1BECC result resets old presentation.

Initialization reset also includes1A14, which the original type1 continuation
wrapper reaches directly at1C98 after advancing its descriptor pointer. That
next-stage path does not re-enter1950, so1950 alone would retain presentation
into the next map. Both verified initialization entries and title/frontend
resets clear it. No native control-flow changes are introduced.

TDD: visibility/CONTINUING assertion failed before the change; then strict tests
passed visibility through exit, no input override after exit, repeated stable
hook not reactivating input, initialization clear, and new-result reset. A separate
1A14-clear assertion failed before adding that native initialization reset and
passed afterward. Pure state and runtime private object both pass C11
-Wall -Wextra -Werror. Root owns the renderer's native fade-lightness treatment;
actual transition frames remain pending its integrated relink.

A focused actual held-START entry probe reproduced a separate guard boundary:
HOLD START from the prepared outcome at frame6000 through9500 skipped the
result on the previous executable and reached the next-stage map/CREATURES
view. This disproved complete release guarding at the first native poll.
Evidence: prompt-held-start/. The input filter had collected START before
stable1BD6C activated the UI, letting that first2B38 consume the old pad.

The verified1BECC candidate now suppresses guest pad input during result drawing,
while presentation stays hidden until1BD6C. Original control flow between these
hooks contains two Vblank waits at1BC5C/1BC60 and two more at1BD54/1BD58, so the
frame filter runs before the first stable native poll. It writes no guest RAM
or controller fields directly. Candidate-held-START unit assertion failed before
this correction and passed afterward; active-held-START still requires release.
Exiting clears candidate and active, retaining normal input passthrough. Actual
held-entry qualification on the corrected copied executable remains pending.

## Corrected native transition and held-entry qualification

Copied integrated executable SHA256
28581cc4804fc2338700b622424ad076916fdc2fc61885f303bc3dbaeaa222fc
passed the previously failing held-START entry probe. START held from prepared
outcome through frame9500 now leaves the result and default Continue panel
visible. After release, read-only assertions confirm stageword21 with the old
result descriptor pointerB3A6, proving no native exit. Only a fresh START press
then continues through the original loader to descriptorB3F0. The run completed
12000 clean frames. Evidence: prompt-held-start-fixed/ and its private input
fixture prompt-held-start-fixed.input.

One English real2560x1440 composed transition run completed12500 frames with
identical input timing to the previous production route. The new CONTINUING
panel remains visible at+3 frames; the old PRESS START region no longer returns.
At+73 and+113 the entire RGB image is black, proving no bright UI remains during
the native blackout. At+203 native STAGE02 START is visible without the panel;
the final native map screenshot is identical to the previous run. Canonical
guest/audio hashes remain CBEE49C7975118FB /97E6BC48FCC4975B exactly.
Automated image comparison found all stable result images and the final map
byte-identical, with the only changed common capture confined to the160x45
Continue panel. Code, statistics and dinosaur sprites remain unchanged.
Evidence: prompt-en-1440-fixed/{probe.json,transition-verification.json,
prompt-continuing.png,prompt-fade-73.png,prompt-fade-113.png,
prompt-fade-203.png,prompt-native-exit.png}. This resolves the previously
recorded transition presentation limitation. The renderer's native fade-lightness
sampling belongs to the root integration changes; no guest state or PC changes
were used in this correction.

## Hard-ending MHRI display: read-only lifecycle qualification

The known additional native code display is the hard-ending nine-row MHRI
message. An actual43000-frame prepared hard-ending route completed cleanly on
copied executable28581cc4804fc2338700b622424ad076916fdc2fc61885f303bc3dbaeaa222fc,
English effective ROMad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261.
The fixture prepares final-stage identity/outcome before allowing original game
ending/credits control flow; it is not a campaign playthrough. No PC jump,
cartridge modification or direct call into credits is used. Public reproduction
fixture: games/db1/scenes/progress-hard-ending-diagnostic.input.
Private evidence: local/progress-agent/hard-ending-research/.

Original code tests completed stageword59 at28E1C..28E26. It selects the message
at28EA2, renders nine rows with29018, and fades it in via28F14. Source28F1A is
after the fade and already validated by progress_session: completed59, native
table60 bytes32 1E 46 22 (MHRI), and canonical target descriptor ordinal1. The
saved target is Back/alien stage1 (host mode3/stage1), not cleared Hard20.

At28F20 native code sets D7=0E0F. Loop28F24 calls770 and DBRA28F2A repeats3600
Vblank waits, about60.08 seconds of guest time. It contains no2B38 input poll.
Fade-out begins28F2E, calls1B538 at28F56, clears display through1ACD8 at28F62,
and returns to restart flow via29DCE. This screen automatically expires; START
is not a native Continue action here. Actual captures at37000,38000,39000 and
40000 show the same message. START at38000 and A at38060 leave subsequent
38060/38120 PNGs byte-identical; by41000 the original intro has resumed.

The inspected native320x224 message outer box occupies approximately(40,24)
through(279,199). The final MHRI row is around y176..184. Ordinary result prompt
at(80,176)160x45 would cover the code. A separate compact Save-only panel can
fit in the existing black margin at(80,201),160x22, ending at y223. Proposed
labels: A: SAVE GAME plus saved/failed feedback. No Continue button should claim
to skip this timed screen. It must preserve the original3600-frame timer and
clear at28F2E before native fade, with the usual reset/title/initialization guards.

Proposed state/runtime extension, not yet implemented: tag this prompt as a
separate timed-ending kind; activate from independently validated source28F1A
after fade; use the same release-before-confirm, single Save edge/retry and atomic
save helper; ignore ordinary two-choice navigation/START confirmation here.
Expose a read-only prompt-kind getter so the root renderer can select the compact
layout. Keep accepted-password1DD90 excluded. Timed expiry clears host input and
presentation without advancing PC or changing any native timer/register/state.

Static inventory bounds: the native password-table absolute pointer occurs only
in shared renderer1BEAE and entry parser1DCF2. The shared renderer1BEA8 has two
verified BSR.w calls: winning1BBE8 and losing1BC08. The hard-message literal
pointer29CE4 has one reference at28EA4. The translation catalog's password text
is this hard-ending message. These checks bound known table-driven and known
literal displays; they do not prove no other embedded/graphic/encoded password
exists. Exhaustive all-screen coverage remains an explicit research limit.

## Timed-ending Save-only state implementation

The timed-ending extension adds a read-only prompt-kind API:0 hidden,1 ordinary
result,2 timed hard-ending. Source28F1A must produce a currently validated hard
ending candidate before this kind activates. Ordinary1BECC and accepted-password
1DD90 remain distinct. The ending presents one Save choice only; Start and
navigation cannot create Continue pulses. A/C confirmation requires release and
one new edge, with the same saved/failed feedback and retry helper as ordinary
results. Native28F2E clears it before automatic fade. No native timer, register,
PC or guest memory field is modified by this state extension.

TDD evidence: kind2 lifecycle assertion failed before implementation, then the
strict pure-state suite passed ending activation/exclusion, held/released START
ignored, Save edge suppression, failed-save retry, successful feedback, expiry
clear and input passthrough. Existing ordinary lifecycle/Continue/held-start tests
remain passing. Runtime compiled independently to a private object with C11
-Wall -Wextra -Werror. Root owns the compact renderer and GameSpec wiring;
actual hard-ending Save capture and native timer/hash comparison remain pending
its integrated relink.

The hard-ending code's native replay was independently checked while waiting
for compact renderer integration. Existing private28581... executable queued
actual ROM table60 bytes321E4622 through the qualified Password route. Original
parser1DD90 wrote stageword003C and canonical pointer00FFBF36; native loader1950
was observed at elapsed67. Read-only final assertions passed and the Back map
was visually inspected after6000 clean frames. Accepted-password entry did not
activate a result Save panel. Evidence: local/progress-agent/replay-mhri/.
This test bypassed host selection with the explicit table-index diagnostic;
it establishes native MHRI continuation, not the forthcoming hard-screen Save
or Extras selection path by itself.

## Actual hard-ending Save qualification

Integrated copied executable
7713c7ccffac7de51a10e5587f88b860e8c0ff869a1525961c596825a3b64d05
completed the exact43000-frame baseline prepared-ending input route with the
compact production Save option and no Save-test environment. START during the
message remained ignored; the subsequent A edge saved exactly once. The105-byte
store has a valid checksum and effective English ROM identity, mode3/stage1,
source028F1A and original encoded bytes321E4622 (MHRI). Save Game and PROGRESS
SAVED footers were visually reviewed beneath the full unchanged nine-row message.

Guest/audio hashes match the previous no-option baseline exactly:
D56BA8F757555B2D / ACD558CC16C3DACA. Every corresponding image differs only
inside native footer rectangle(80,201)..(240,223); all message/code pixels above
it are identical. Earlier credits and later41000/42000 intro frames are entirely
identical, with no footer after automatic expiry. This confirms the original
3600-Vblank display duration and native restart flow remain unaffected.
Private evidence: hard-ending-save/{probe.json,run.log,verification.json,
hard-secret-38000.png,hard-secret-after-a.png,profile/}. Verification decoded the
actual file, checked its checksum/native target/code/source and compared every
captured frame against the baseline. The input fixture remains explicitly a
prepared hard completion, not a campaign playthrough.

Known displayed-code Save coverage now includes shared ordinary winning/losing
renderer1BECC and the separately qualified timed hard-ending MHRI source28F1A.
Manual accepted-password1DD90 remains excluded from result prompts. Unknown
literal/graphic code displays still require discovery; this does not turn the
bounded static inventory into an exhaustive all-secret-screen claim.


## UTF-8 Windows progress paths

A focused native probe reproduced a misleading success: narrow fopen/MoveFileExA returned success for UTF-8保存.bin, but GetFileAttributesW could not find the intended filename. A regression asserted the intended Unicode filename and failed before the fix. Progress storage now converts validated UTF-8 to UTF-16 for open, exclusive temporary creation, atomic replacement and cleanup. Existence checks use the same encoding and fail closed on access/encoding errors, so runtime startup does not ignore an unreadable existing save.

The fixed regression passes Unicode save/load, the exact intended name, locked replacement preserving prior data, deletion/absence and malformed-UTF-8 rejection. Existing identity/corruption/ASCII replacement checks remain passing. This is Windows storage evidence, not physical device or campaign acceptance.
