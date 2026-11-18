import hashlib
import struct
import unittest
from tools.translate_db2 import translate


class TranslationTests(unittest.TestCase):
    def setUp(self):
        self.source = bytes(600) + b'JP_TEXT' + bytes(417)
        self.catalog = dict(source_sha256=hashlib.sha256(self.source).hexdigest(), entries=[
            dict(address=600, capacity=7, before=b'JP_TEXT'.hex(), english='HELLO__')])

    def test_guarded_edit_keeps_layout_and_checksum(self):
        result = translate(self.source, self.catalog)
        self.assertEqual(len(result), len(self.source))
        self.assertEqual(result[600:607], b'HELLO__')
        self.assertEqual(self.source[600:607], b'JP_TEXT')
        checksum = sum(struct.unpack('>256H', result[512:])) & 65535
        self.assertEqual(int.from_bytes(result[0x18e:0x190], 'big'), checksum)

    def test_wrong_rom_and_bad_catalog_rejected(self):
        with self.assertRaises(ValueError):
            translate(bytes(len(self.source)), self.catalog)
        self.catalog['entries'][0]['english'] = 'TOO_LONG_FOR_SLOT'
        with self.assertRaises(ValueError):
            translate(self.source, self.catalog)

    def test_short_text_no_longer_silently_padded(self):
        self.catalog['entries'][0]['english'] = 'HELLO'
        with self.assertRaises(ValueError):
            translate(self.source, self.catalog)

    def test_rejects_out_of_bounds_guard_and_control_char_entries(self):
        cases = [
            dict(address=500, capacity=7, before=b'JP_TEXT'.hex(), english='HELLO__'),
            dict(address=600, capacity=5, before=b'JP_TEXT'.hex(), english='HELLO'),
            dict(address=1020, capacity=7, before=b'JP_TEXT'.hex(), english='HELLO__'),
            dict(address=600, capacity=7, before=b'WRONG__'.hex(), english='HELLO__'),
            dict(address=600, capacity=7, before=b'JP_TEXT'.hex(), english='HELLO\n_'),
        ]
        for entry in cases:
            with self.subTest(entry=entry):
                catalog = dict(source_sha256=self.catalog['source_sha256'], entries=[entry])
                with self.assertRaises(ValueError):
                    translate(self.source, catalog)

    def test_rejects_overlapping_edits(self):
        source = bytes(600) + b'JP_TEXTJP_TEXT' + bytes(410)
        catalog = dict(source_sha256=hashlib.sha256(source).hexdigest(), entries=[
            dict(address=600, capacity=10, before=source[600:610].hex(), english='A' * 10),
            dict(address=605, capacity=9, before=source[605:614].hex(), english='B' * 9)])
        with self.assertRaises(ValueError):
            translate(source, catalog)
