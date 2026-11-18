# Pure native Password navigation

The host requests navigation only from the separately verified mature DB1 title
menu, with its read-only native selection0..3. Outside it, request fails with
return-to-title guidance. No CPU, RAM, ROM, replay or UI pointers are owned.

request first absorbs the button that closed Extras and waits for full physical
release. Native0 uses Up to wrap to3;1 uses Down twice;2 uses Down once;3 needs
no direction. Each direction is3 frames pressed and3 released, followed by
acknowledgement of the expected native selection before the next step. Unexpected
selection changes, native demo, Options, game dispatch and known scene exits
cancel. Missing acknowledgement never causes another pulse.

Confirmation requires an exact physical A/B/C mapping supplied by the caller:
decode the first RAM0028..002A entry whose value equals1, as the existing replay
does. The enum is abstract physical A=1,B=2,C=4,Up=8,Down=16,Other=32. Translate
to/from GPAD; never pass native logical action values as physical button masks.
All physical inputs, including Start/directions/other controls, must reach frame
for release/cancellation, not just confirmation. Confirmation is3 pressed and3
released;1D7B6 is the observed handoff and stops navigation immediately. The
module never changes guest program counter or native menu selection.

Root integration order:

1. Queue the already qualified replay request, then request navigation from the
   mature getter/current native anchor. Outside title cancel the queued replay and reopen Extras with guidance.
2. Call before with observed hooks and valid native physical remap, before the
   replay observation. At1D7B6 navigation relinquishes ownership before prefill.
3. While owns is true suspend the virtual Extras filter and replay_frame. Call
   nav frame once per guest frame with real input; use its exclusive pad output.
4. Cancel the pending replay when navigation cancels/times out. Keep calling nav
   frame while owns remains true: cancellation and timeout drain held controls
   until full release, rather than passing A/Start into the original menu.
5. After successful handoff resume the existing qualified replay path. No custom
   parser, loader or guest-control-flow path is added.

A300-frame navigation limit is below replay's1800-frame budget. Pause replay's
frame clock while navigation owns, including terminal release guard; do not
restart or duplicate replay requests per frame. Held-input protection may outlast
300frames after terminal status; it emits zero pad and cannot progress a scene.

Pure strict C11 tests cover all four starting anchors, exact3+3 pulses, held modal
confirmation, delayed acknowledgement, unexpected drift, invalid/changing remap,
user cancellation, terminal held-button drain, native exits, unsolicited Password
entry, timeout, and successful observed handoff. Native integrated navigation and
physical remapping still require root qualification; pure tests are not a native
ROM playthrough.
