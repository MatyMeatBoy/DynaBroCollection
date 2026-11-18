# Guest clock and real-time countdown qualification

The NTSC-J machine clock is 53,693,175 master ticks/second, 3420 ticks/line
and 262 lines/frame: **59.92274340431231 guest frames/second**. Independent
Genesis Plus GX `retro_get_system_av_info` returned exactly that frame rate
for the original Japanese DB1 dump. A 1200-frame interval is 20.0257854 seconds.

The prior runner default used the television video ratio 60000/1001 instead,
59.94005994 Hz. That is 0.028898% faster than the machine clock (approximately
1.04 seconds per hour). The host deadline should follow the guest master clock;
an explicit diagnostic frame-rate override remains an override.

## Measured before the host-clock correction

Fresh isolated settings, same audio-fixed development executable SHA-256
`57e3cf2d6f96331bf2e68baf78d8abf225999ff561f4ca419204542eb2f5582c`,
original protected Japanese ROM, 7202 **paced runtime** frames. No uncapped
runtime flag was used. Original selected 4:3 and standard presentation; Enhanced
selected 16:9 and 144 Hz presentation. SDL's dummy output and hidden window
kept host audio/device differences out of this simulation-clock measurement.

| Run | Timer FF7BB4 at frame 4800 | At 6000 | At 7200 | Wall time 6000–7200 |
| --- | ---: | ---: | ---: | ---: |
| Native Original | 54000 | 53501 | 52919 | 20.0210848 s |
| Native Enhanced | 54000 | 53501 | 52919 | 20.0192449 s |
| Independent GPGX original | 54000 | 53516 | 52933 | 20.0253008 s |

Native RAM was read as big-endian longs. GPGX exposes work RAM with bytes
swapped within each 16-bit word; the table normalizes that representation.
Native times use RAM-dump file modification timestamps. Oracle times use
`perf_counter` after each `retro_run`, with actual deadline pacing from frame
4800 onward. Oracle boot before that frame was fast-forwarded; its measured
2400-frame section ran for more than 40 wall seconds.

Both native modes produced the same countdown values and identical canonical
game hash `D4A189867C9C1EB4` and audio hash `08214B0251C1339C` at frame7202.
Their full runtime measurements were 120.155255 and 120.1578282 seconds.
Enhanced presentation therefore did not add guest frames or countdown updates
in this tested workload. The comparison uses a full real-time 20-second battle
window, rather than inferring game speed from uncapped benchmark throughput.

The stable battle window drops 582 native timer ticks versus 583 oracle ticks.
This is within one integer countdown boundary over the window; native battle
start is approximately 15 countdown ticks earlier in this input sequence.
This observation does **not** establish frame-exact equivalence to the original
machine across stage transitions or complete campaigns.

Evidence is in ignored local folders `local/timing-native-db1-original`,
`local/timing-native-db1-enhanced` and `local/timing-reference-db1`. Reference
core SHA-256:
`47dbded6c567a0498ba7eb78a110c1f363d417d64a6afbbe3c5dbfb44d94a576`.
Input was START at frames2000/4012 and A at4512, held for12 frames each.

## Pacing-mode distinction

The shared engine initializes `g_pacing_mode` to `FIBER_FULL`; its comment says
that choice is intentional for another game's cycle-cost investigation. A
nearby runner comment incorrectly describes `CYCLE_ACCURATE` as the compiled
default. These measurements deliberately did not pass `--pacing`, so they
exercise the actual compiled default. Host frame cadence and the CPU instruction
budget policy are distinct settings. This test supplies no basis for changing
the shared engine policy globally.

Physical display cadence, speaker playback, DB2 countdowns and complete campaign
equivalence remain outside this particular measurement.


## Integrated accurate-default check, 0.1.4 candidate

1200 paced boot frames (default pacing, no CLI FPS override) took20.0298730s Original and20.0288767s Enhanced, against20.0257854s at53693175/896040Hz. Both modes produced state882012888446801C and audioFB5F8BB3A53A9906. Evidence local/timing-integrated-014/report.json. This checks the new default pacer and equal presentation modes; it does not replace the earlier battlefield countdown comparison or assert a complete campaign timing audit. Dummy audio device was used, so queue drops are not physical playback evidence.
