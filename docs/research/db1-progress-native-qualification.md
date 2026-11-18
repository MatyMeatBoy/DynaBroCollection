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
