import array
import math
from pathlib import Path
import tempfile
import unittest
import wave
from tools.audio_capture import inspect_audio_capture


class AudioCaptureTests(unittest.TestCase):
    def inspect(self, left, right):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'capture.wav'
            with wave.open(str(path),'wb') as output:
                output.setparams((2,2,8000,0,'NONE','not compressed'))
                pcm=array.array('h',(sample for pair in zip(left,right) for sample in pair))
                output.writeframes(pcm.tobytes())
            return inspect_audio_capture(path)

    def test_rejects_dc_even_with_startup_transient_and_channel_offset(self):
        left=[55]*800+[380]*23200
        result=self.inspect(left,[value+500 for value in left])
        self.assertFalse(result['signal_present'])

    def test_accepts_ac_signal_after_silent_boot(self):
        signal=[0]*8000+[int(2000*math.sin(i*0.1)) for i in range(16000)]
        self.assertTrue(self.inspect(signal,signal)['signal_present'])
