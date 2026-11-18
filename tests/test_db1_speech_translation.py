import hashlib
import os
from pathlib import Path
import unittest
from tools import db1_speech_translation as speech

class SpeechTranslationTests(unittest.TestCase):
    def test_db1_codec_overlapping_copy(self):
        raw,used=speech.decode(bytes.fromhex('0200000041ff00ff00ff00fe00'))
        self.assertEqual(raw,b'A'*512);self.assertEqual(used,13)
    def test_codec_roundtrips_and_is_bounded(self):
        for raw in [b'A'*512,bytes(range(256))*2,bytes(range(128))*4,bytes((i*17+i//7)%256 for i in range(512))]:
            packed=speech.encode(raw)
            self.assertEqual(speech.decode(packed),(raw,len(packed)))
        for invalid in [b'',bytes.fromhex('000000'),bytes.fromhex('0200008000'),bytes.fromhex('0200007f4142')]:
            with self.assertRaises(ValueError):speech.decode(invalid)
    def test_pixels_and_interior_only(self):
        original=bytes((i*37)%256 for i in range(512))
        for frame in speech.FRAMES:
            result=speech.replace_letters(original,frame)
            mask=speech.interior(frame)
            self.assertEqual(len(result),512)
            self.assertNotEqual(original,result)
            for y in range(32):
                for x in range(32):
                    if (x,y) not in mask:self.assertEqual(speech.pixel(original,x,y),speech.pixel(result,x,y))
            self.assertEqual({speech.pixel(result,x,y) for x,y in mask},{1,frame.ink})
    def test_bad_sizes_coordinates_and_text_rejected(self):
        with self.assertRaises(ValueError):speech.replace_letters(b'bad',speech.FRAMES[0])
        with self.assertRaises(ValueError):speech.encode(b'')
        with self.assertRaises(ValueError):speech.pixel(bytes(512),32,0)
    def test_unqualified_source_rejected(self):
        with self.assertRaises(ValueError):speech.translate(bytes(1048576))
    @unittest.skipUnless(os.environ.get('DB1_SPEECH_TEST_ROM'),'private ROM qualification opt-in')
    def test_real_qualified_source_all_changes_scoped(self):
        source=Path(os.environ['DB1_SPEECH_TEST_ROM']).read_bytes()
        result,report=speech.translate(source)
        self.assertEqual(len(source),len(result))
        allowed={0x18e,0x18f}
        for frame in speech.FRAMES:
            before,used=speech.decode(source,frame.address);after,new_used=speech.decode(result,frame.address)
            self.assertEqual(used,frame.capacity);self.assertLessEqual(new_used,frame.capacity)
            self.assertEqual(after,speech.replace_letters(before,frame))
            # Independent fixed regions: every actor/tail row and contour row.
            for y in [0]+list(range(13,32)):
                for x in range(32):self.assertEqual(speech.pixel(before,x,y),speech.pixel(after,x,y))
            for y,(left,right) in enumerate(frame.bounds,1):
                self.assertEqual(speech.pixel(before,left,y),15)
                self.assertEqual(speech.pixel(before,right,y),15)
                for x in (left,right):self.assertEqual(speech.pixel(before,x,y),speech.pixel(after,x,y))
            # The visible hand in frame30 begins above the generic actor rows.
            if frame.frame==0x30:
                for x in range(32):self.assertEqual(speech.pixel(before,x,11),speech.pixel(after,x,11))
            allowed.update(range(frame.address,frame.address+frame.capacity))
        self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(source,result))))
        self.assertEqual(report['source_sha256'],hashlib.sha256(source).hexdigest())

if __name__=='__main__':unittest.main()


