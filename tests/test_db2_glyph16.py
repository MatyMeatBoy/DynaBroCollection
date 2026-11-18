import hashlib
import unittest

from tools.translate_db2 import glyph16, translate


class Glyph16Test(unittest.TestCase):
    def test_letters_digits_punctuation(self):
        self.assertEqual(glyph16('Ab 1.', 5), bytes.fromhex('00e1 00e2 0001 00d1 0005'.replace(' ', '')))

    def test_padding_and_alignment(self):
        self.assertEqual(glyph16('A', 3, 'center'), bytes.fromhex('000100e10001'))
        self.assertEqual(glyph16('A', 3), bytes.fromhex('00e100010001'))

    def test_overflow_and_unsupported(self):
        with self.assertRaises(ValueError):
            glyph16('ABCD', 3)
        with self.assertRaises(ValueError):
            glyph16("A'", 3)

    def test_entry_guards_and_nul_word(self):
        source = bytearray(1024)
        source[600:606] = bytes.fromhex('018a01ca0198')
        source[606:608] = bytes(2)
        catalog = dict(source_sha256=hashlib.sha256(source).hexdigest(), entries=[
            dict(address=600, capacity=6, before='018a01ca0198', english='OK', encoding='glyph16')])
        out = translate(bytes(source), catalog)
        self.assertEqual(out[600:606], bytes.fromhex('00ef00eb0001'))
        self.assertEqual(out[606:608], bytes(2))
        catalog['entries'][0]['before'] = '018a01ca0199'
        with self.assertRaises(ValueError):
            translate(bytes(source), catalog)
        source[606:608] = b'\x01\x01'
        catalog['source_sha256'] = hashlib.sha256(source).hexdigest()
        catalog['entries'][0]['before'] = '018a01ca0198'
        with self.assertRaises(ValueError):
            translate(bytes(source), catalog)


if __name__ == '__main__':
    unittest.main()
