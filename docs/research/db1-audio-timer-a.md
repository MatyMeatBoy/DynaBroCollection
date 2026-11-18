# DB1 missing audio: Timer A status

The immutable v0.1.3 portable DB1 executable reproduces silent audio on this
machine with a fresh isolated profile, the original Japanese ROM and the
6100-frame `debug/db1-16x9.input` scene. This is a mixer-side defect independent
of the other PC's physical playback device.

The uploaded Z80 sound driver runs at PCs $0075–$008F, polls YM2612 status bit
0 and calls its sequencer tick only when Timer A overflows. The bus modeled
Timer B only, so Timer A's flag never became set. FM/PSG sample counts remained
nonzero despite their samples containing only a steady DC level. A successful
finite runtime probe therefore did not establish audible sound.

The bus now models Timer A's 10-bit reload ($24/$25), start bit 0, overflow flag
enable bit 2 and acknowledgement bit 4 in $27. Its period is
`(1024 - reload) * 144 * 7` master cycles. Acknowledging a running timer preserves
its phase. Timer B retains its prior behavior. Timer A's scalar guest state is
included in the existing machine rollback image; old layout images fail the
existing size check.

Evidence (local ignored outputs, not release artifacts):

| Capture | Active one-second AC windows | Peak AC RMS | PCM range |
| --- | ---: | ---: | --- |
| `local/audio-v013-fresh-battle/audio.wav` | 0 / 102 | 2.93 | 55 to 380 |
| `local/audio-fixed-battle/audio.wav` | 88 / 102 | 2187.48 | -7277 to 9597 |

The original ROM SHA-256 is
`b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c`.
Both runs completed 6100 frames. The first capture used the portable package's
own DLLs and a PATH restricted to Windows system directories. SDL's dummy audio
backend and WAV tap prove mixer generation; they do not qualify physical speaker
output, Windows session volume, device selection or audible fidelity. The fixed
development binary required the development DLL path, so release packaging must
repeat verification against its own bundled DLLs.

`tests/audio_timer_test.c` exercises the real CPU-visible bus with a controlled
clock. It failed on the absent Timer A overflow before the fix and passes after
it; it also checks acknowledgement, stopping, register masking and the existing
Timer B phase test. `tools/audio_capture.py` rejects silence and steady DC even
with a startup transient; two unit tests cover those cases and a real AC signal.
