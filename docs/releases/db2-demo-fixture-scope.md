# DB2 fixture scope correction — discovered after0.1.7 publication

The historical `unpause-gameplay.input` waits1999 frames, presses Start, then waits2000 before the next Start. Read-only tracing of the exact0.1.7 native executable shows the resulting moving world belongs to the attract/demo driver, not a positively established player-controlled campaign battle.

Evidence: native mode/stage fields atFF027C are08/06 in the captured world. F6EA chooses demo group8/stage5 plus the demo index; F722 calls F794, whose Start-abort handler is F768 and whose return cleanup is F728. F93A compares the demo timer to3000 frames before returning. The saved stack contains the F728 return and the minimap/display callback return; read-only sampling observes the DB2 battle latch remaining0 throughout. The ordinary campaign3BB0→3DAC→3BB4 wrapper is not entered by that fixture. Valid terrain, moving actors, a changing counter, a complete benchmark and equal hashes alone did not establish player control.

The finite IRQ/replay/runtime comparisons remain real tests of that native demo route. Their observed hashes and artifact identities are unchanged. They do not certify campaign input, ordinary battle lifecycle, player-triggered hazards or working expanded-world rendering. Earlier notes describing this route as active gameplay or unpause acceptance overstated its scope. The case name `db2-unpause` in immutable manifests remains historical; it is not a new claim of interactive acceptance.

This is an evidence correction, not a newly demonstrated cartridge failure. Published archives remain unchanged. New development separately qualifies an explicit demo renderer lifecycle and an actual player-controlled Story/battle route, with native entry/exit observations and movement postconditions. Neither silently substitutes for the other.

Private retained evidence: local/db2-world-agent/lifecycle-readonly, including source disassembly, captured fields/stack and read-only own-process sampling. No ROM, capture or private log is redistributed here.
