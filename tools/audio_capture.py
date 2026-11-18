"""Measure captured mixer PCM; this does not verify physical speaker output."""
import array
import math
import sys
import wave


def inspect_audio_capture(path):
    """Reject silence and steady DC, including captures with a boot transient."""
    active = 0
    windows = 0
    peak_ac_rms = 0.0
    with wave.open(str(path), 'rb') as source:
        if source.getsampwidth() != 2 or source.getnchannels() != 2:
            raise ValueError('Expected stereo 16-bit mixer capture')
        rate = source.getframerate()
        frames = source.getnframes()
        while True:
            pcm = array.array('h', source.readframes(rate))
            if not pcm:
                break
            if sys.byteorder != 'little':
                pcm.byteswap()
            # Sampling at ~14 kHz keeps verification cheap even for the
            # mixer's 223721 Hz files. Calculate each channel independently
            # so a fixed left/right DC difference cannot count as sound.
            ac_squared = 0.0
            for channel in range(2):
                samples = pcm[channel::32]
                mean = sum(samples) / len(samples)
                ac_squared += sum((sample-mean)**2 for sample in samples) / len(samples)
            rms = math.sqrt(ac_squared / 2)
            peak_ac_rms = max(peak_ac_rms, rms)
            windows += 1
            active += rms >= 32.0
    return dict(frames=frames, sample_rate=rate, windows=windows,
                active_windows=active, peak_ac_rms=peak_ac_rms,
                signal_present=active >= 2,
                scope='Mixer PCM only; physical playback is unverified')
