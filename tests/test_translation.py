import hashlib
import json
from pathlib import Path
import struct
import unittest
from tools.translate_db2 import translate

ROOT = Path(__file__).resolve().parent.parent


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

    def test_real_catalog_entries_are_self_consistent(self):
        catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
        entries = catalog['entries']
        self.assertEqual(len(entries), 22)
        used = set()
        for entry in entries:
            address, capacity = entry['address'], entry['capacity']
            before = bytes.fromhex(entry['before'])
            english = entry['english'].encode('ascii')
            self.assertGreaterEqual(address, 512)
            self.assertEqual(len(before), capacity)
            self.assertEqual(len(english), capacity)
            self.assertTrue(all(x >= 32 or x == 1 for x in english))
            span = set(range(address, address + capacity))
            self.assertFalse(span & used, f'overlapping entries at {address:06X}')
            used |= span
